#include <Python.h>
#include <pybind11/pybind11.h>
#include <fmt/format.h>
#include <atomic>
#include <thread>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <string_view>
#include <charconv>
#include <filesystem>
#include <describe/describe.hpp>
#include "sl_lidar.h"

namespace py = pybind11;
namespace fs = std::filesystem;

namespace lidarbridge
{

using std::string;
using std::string_view;
using std::vector;
using std::map;
using std::unique_ptr;

//"serial:/dev/ttyACM0?baud=115200"
struct Uri {
    string scheme;
    string path;
    map<string, string, std::less<>> params;

    static Uri Parse(string_view src) {
        auto next = [&](char sep) {
            auto found = src.find(sep);
            auto res = src.substr(0, found);
            if (found != string_view::npos) {
                src = src.substr(found + 1);
            } else {
                src = {};
            }
            return res;
        };
        Uri res;
        res.scheme = string{next(':')};
        res.path = string{next('?')};
        string_view param;
        while((param = next('=')).size()) {
            res.params[string{param}] = string{next('&')};
        }
        return res;
    }
};

template<typename E>
string_view PrintEnum(E e) {
    string_view res;
    if (!describe::enum_to_name(e, res)) {
        res = "<invalid>";
    }
    return res;
}

template<typename...Args>
auto Err(string_view fmt, Args const&...a) {
    return std::runtime_error(fmt::format(fmt, a...));
}

struct Spinner {
    virtual ~Spinner() = default;
};

enum MinimumIds {
    LIDAR_A_SERIES_MINUM_MAJOR_ID = 0,
    LIDAR_S_SERIES_MINUM_MAJOR_ID = 5,
    LIDAR_T_SERIES_MINUM_MAJOR_ID = 8,
};

enum Results : int64_t {
    ResultOk = 0,
    ResultAlreadyDone = 0x20,
    ResultInvalidData = (0x8000 | SL_RESULT_FAIL_BIT),
    ResultOperationFail = (0x8001 | SL_RESULT_FAIL_BIT),
    ResultOperationTimeout = (0x8002 | SL_RESULT_FAIL_BIT),
    ResultOperationStop = (0x8003 | SL_RESULT_FAIL_BIT),
    ResultOperationNotSupported = (0x8004 | SL_RESULT_FAIL_BIT),
    ResultFormatNotSupported = (0x8005 | SL_RESULT_FAIL_BIT),
    ResultInsufficientMemory = (0x8006 | SL_RESULT_FAIL_BIT),
};

DESCRIBE(lidarbridge::Results, ResultOk,ResultAlreadyDone,ResultInvalidData
,ResultOperationFail,ResultOperationTimeout,ResultOperationStop
,ResultOperationNotSupported,ResultFormatNotSupported,ResultInsufficientMemory)

enum ModelFamily {
    Unknown = 0,
    SeriesA,
    SeriesS,
    SeriesT,
};
DESCRIBE(lidarbridge::ModelFamily, Unknown,SeriesA,SeriesS,SeriesT)

template<typename Map, typename T>
T GetOr(Map const& m, string_view k, T adef) {
    if (auto it = m.find(k); it != m.end()) {
        if constexpr (std::is_integral_v<T>) {
            auto r = std::from_chars(it->second.data(), it->second.data() + it->second.size(), adef);
            if (r.ec != std::errc{}) {
                throw Err("Invalid param: {}", k);
            }
            return adef;
        } else {
            return it->second;
        }
    } else {
        return adef;
    }
}

static sl::IChannel* connect(Uri const& uri) {
    if (uri.scheme == "serial") {
        if (!fs::exists(uri.path)) {
            throw Err("Port does not exist: {}", uri.path);
        }
        return *sl::createSerialPortChannel(uri.path, GetOr(uri.params, "baud", 256400));
    } else {
        throw Err("Unsupported scheme: {}", uri.scheme);
    }
}


static void checkHealth(sl::ILidarDriver& driver)
{
    sl_lidar_response_device_health_t healthinfo;
    if (auto err = Results(driver.getHealth(healthinfo)); err & SL_RESULT_FAIL_BIT) {
        throw Err("Could not fetch health: {}", PrintEnum(err));
    }
    switch (healthinfo.status) {
    case SL_LIDAR_STATUS_OK:
        return;
    case SL_LIDAR_STATUS_WARNING:
        throw Err("RPLidar health status : Warning.");
    case SL_LIDAR_STATUS_ERROR:
        throw Err("Error, rplidar internal error detected. Please reboot the device to retry.");
    default:
        throw Err("Invalid health status");
    }
}

struct LidarInfo {
    ModelFamily series;
    bool needsTune{};
    string sn;
    string mode;
    uint16_t fwmaj;
    uint16_t fwmin;
    uint16_t hwver;
};

static LidarInfo getInfo(sl::ILidarDriver& driver) {
    LidarInfo info;
    sl_lidar_response_device_info_t devinfo;
    if (auto err = Results(driver.getDeviceInfo(devinfo)); err & SL_RESULT_FAIL_BIT) {
        throw Err("Could not get device info: {}, errno: {}", PrintEnum(err), errno);
    }
    char sn_str[35] = {0};
    for (int pos = 0; pos < 16 ;++pos) {
        sprintf(sn_str + (pos * 2),"%02X", devinfo.serialnum[pos]);
    }
    char mode_str[16] = {0};
    info.needsTune = true;
    if((devinfo.model>>4) <= LIDAR_S_SERIES_MINUM_MAJOR_ID){
        sprintf(mode_str,"A%dM%d",(devinfo.model>>4),(devinfo.model&0xf));
        info.series = SeriesA;
        info.needsTune = false;
        driver.setMotorSpeed(600);
    }else if((devinfo.model>>4) <= LIDAR_T_SERIES_MINUM_MAJOR_ID){
        sprintf(mode_str,"S%dM%d",(devinfo.model>>4)-LIDAR_S_SERIES_MINUM_MAJOR_ID,(devinfo.model&0xf));
        info.series = SeriesS;
    }else{
        sprintf(mode_str,"T%dM%d",(devinfo.model>>4)-LIDAR_T_SERIES_MINUM_MAJOR_ID,(devinfo.model&0xf));
        info.series = SeriesT;
    }
    info.mode = mode_str;
    info.sn = sn_str;
    info.fwmaj = devinfo.firmware_version>>8;
    info.fwmin = devinfo.firmware_version & 0xFF;
    info.hwver = (int)devinfo.hardware_version;
    return info;
}

static void initMode(sl::ILidarDriver& driver, string_view wanted) {
    std::vector<sl::LidarScanMode> allSupportedScanModes;
    driver.getAllSupportedScanModes(allSupportedScanModes);
    for (auto &mode : allSupportedScanModes) {
        if (string_view{mode.scan_mode} == wanted) {
            driver.startScan(false, mode.id, 0, &mode);
            return;
        }
    }
    string all;
    for (auto& mode: allSupportedScanModes) {
        all += fmt::format(", {}", string_view{mode.scan_mode});
    }
    throw Err("Could not initialize mode: {}, all: [{}]", wanted, all);
}

struct ParsedNode {
    float range{};
    float intensity{};
    float relTheta{};
};

static float toRadians(float degrees)
{
    return (degrees * 6.283f / 360.f);
}

struct Driver {
    std::atomic<bool> shutdown = false;
    std::thread thread;
    LidarInfo info;
    py::function cb;
    std::unique_ptr<sl::ILidarDriver> driver;
    std::unique_ptr<sl::IChannel> chan;

    Driver(Uri const& uri) : driver(*sl::createLidarDriver()) {
        chan.reset(connect(uri));
        auto result = Results(driver->connect(chan.get()));
        if (result & SL_RESULT_FAIL_BIT) {
            throw Err("Could not connect: {}", PrintEnum(result));
        }
        info = getInfo(*driver);
        checkHealth(*driver);
        initMode(*driver, GetOr(uri.params, "mode", string{"DenseBoost"}));
        if (!info.needsTune) {
            driver->setMotorSpeed(GetOr(uri.params, "rpm", 600));
        }
        thread = std::thread(&Driver::spin, this);
    }
    void runCb(sl_lidar_response_measurement_node_hq_t* nodes, size_t count) {
        py::gil_scoped_acquire lock;
        py::tuple result(count);
        for (size_t i = 0; i < count; ++i) {
            ParsedNode node;
            node.range = static_cast<float>(nodes[i].dist_mm_q2/4000.f);
            node.intensity = static_cast<float>(nodes[i].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
            node.relTheta = toRadians(nodes[i].angle_z_q14 * 90.f / 16384.f);
            result[i] = py::make_tuple(node.range, node.intensity, node.relTheta);
        }
        cb(result);
    }
    void spin() {
        size_t count = 8192UL;
        std::vector<sl_lidar_response_measurement_node_hq_t> nodes(count);
        while (!shutdown.load(std::memory_order_relaxed)) {
            if (driver->grabScanDataHq(nodes.data(), count) & SL_RESULT_FAIL_BIT) {
                continue;
            }
            if (driver->ascendScanData(nodes.data(), count) & SL_RESULT_FAIL_BIT) {
                continue;
            }
            runCb(nodes.data(), count);
        }
    }
    ~Driver() {
        shutdown = true;
        if (thread.joinable()) {
            thread.join();
        }
    }
};

static unsigned handles = 0;
static map<unsigned, Driver*> all;

static void stop(unsigned handle) {
    auto it = all.find(handle);
    if (it == all.end()) {
        throw Err("Invalid handle: {}", handle);
    }
    all.erase(it);
}

static unsigned start(std::string rawuri, py::function cb) {
    auto uri = Uri::Parse(rawuri);
    auto id = handles++;
    all[id] = new Driver(uri);
    return id;
}

} //lidarbridge

PYBIND11_MODULE(lidarbridge, m) {
    m.def("start", &lidarbridge::start, "start listening for msgs", py::arg("uri"), py::arg("callback"));
    m.def("stop", &lidarbridge::stop, "stop listening for msgs by handle", py::arg("handle"));
    auto sdk = m.def_submodule("sdk");
    sdk.attr("major") = SL_LIDAR_SDK_VERSION_MAJOR;
    sdk.attr("minor") = SL_LIDAR_SDK_VERSION_MINOR;
    sdk.attr("patch") = SL_LIDAR_SDK_VERSION_PATCH;
}
