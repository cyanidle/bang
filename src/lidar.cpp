#include <Python.h>
#include <pybind11/pybind11.h>
#include <atomic>
#include <thread>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <string_view>
#include "sl_lidar.h"
#include "uri.hpp"

using namespace bang;
namespace py = pybind11;

namespace lidar
{

namespace rp {

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

DESCRIBE(lidar::rp::Results, ResultOk,ResultAlreadyDone,ResultInvalidData
,ResultOperationFail,ResultOperationTimeout,ResultOperationStop
,ResultOperationNotSupported,ResultFormatNotSupported,ResultInsufficientMemory)

enum ModelFamily {
    Unknown = 0,
    SeriesA,
    SeriesS,
    SeriesT,
};
DESCRIBE(lidar::rp::ModelFamily, Unknown,SeriesA,SeriesS,SeriesT)

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

static constexpr float toRadians(float degrees) noexcept
{
    return (degrees * 6.283f / 360.f);
}

struct Driver {
    std::atomic<bool> shutdown = false;
    std::thread thread;
    LidarInfo info;
    std::unique_ptr<sl::ILidarDriver> driver;
    std::unique_ptr<sl::IChannel> chan;
    int rpm = 600;

    Driver(string rawuri) :
        driver(*sl::createLidarDriver())
    {
        auto uri = Uri::Parse(rawuri);
        chan.reset(connect(uri));
        auto result = Results(driver->connect(chan.get()));
        if (result & SL_RESULT_FAIL_BIT) {
            throw Err("Could not connect: {}", PrintEnum(result));
        }
        info = getInfo(*driver);
        checkHealth(*driver);
        initMode(*driver, GetOr(uri.params, "mode", string{"DenseBoost"}));
        rpm = GetOr(uri.params, "rpm", rpm);
        if (!info.needsTune) {
            driver->setMotorSpeed(rpm);
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
        scan(result);
    }
    void spin() {
        size_t count = 8192UL;
        std::vector<sl_lidar_response_measurement_node_hq_t> nodes(count);
        while (!shutdown.load(std::memory_order_relaxed)) {
            if (auto err = Results(driver->grabScanDataHq(nodes.data(), count)); err & SL_RESULT_FAIL_BIT) {
                py::gil_scoped_acquire lock;
                error(fmt::format("AscendScan: {}", PrintEnum(err)));
                continue;
            }
            if (std::exchange(info.needsTune, false)) {
                driver->setMotorSpeed(rpm);
                continue;
            }
            if (auto err = Results(driver->ascendScanData(nodes.data(), count)); err & SL_RESULT_FAIL_BIT) {
                py::gil_scoped_acquire lock;
                error(fmt::format("AscendScan: {}", PrintEnum(err)));
                continue;
            }
            runCb(nodes.data(), count);
        }
    }

    virtual void scan(py::tuple) = 0;
    virtual void error(string msg) {
        py::print("[!] RPLidar: Error: ", msg);
    }

    virtual ~Driver() {
        shutdown = true;
        if (thread.joinable()) {
            thread.join();
        }
        driver.reset();
        chan.reset();
    }
};

struct PyDriver : Driver {
    using Driver::Driver;

    void scan(py::tuple data) override {
        PYBIND11_OVERRIDE_PURE(void, Driver, scan, data);
    }
    void error(string msg) override {
        PYBIND11_OVERRIDE(void, Driver, error, msg);
    }
};

} //rp
} //lidarbridge

PYBIND11_MODULE(lidar, m) {
    auto cls = py::class_<lidar::rp::Driver, lidar::rp::PyDriver>(m, "RP")
        .def(py::init<std::string>(),
                        "Create Driver with specified device URI",
                        py::arg("uri"))
        .def("scan", &lidar::rp::Driver::scan,
                        "Override to handle scan data tuple[range, intensity, theta]",
                        py::arg("data"))
        .def("error", &lidar::rp::Driver::error,
                        "Override to handle error messages",
                        py::arg("msg"));
    cls.attr("vmajor") = SL_LIDAR_SDK_VERSION_MAJOR;
    cls.attr("vminor") = SL_LIDAR_SDK_VERSION_MINOR;
    cls.attr("vpatch") = SL_LIDAR_SDK_VERSION_PATCH;
}
