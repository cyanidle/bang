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
#include <boost/asio/serial_port.hpp>
#include <boost/endian.hpp>
#include "uri.hpp"

namespace py = pybind11;
namespace asio = boost::asio;
using namespace bang;

namespace arduino {

enum Flags : uint16_t {
    Request = 1,
};

struct SLIP {
    static constexpr char END = char(0xC0);
    static constexpr char ESC = char(0xDB);
    static constexpr char EscapedEnd = char(0xDC);
    static constexpr char EscapedEsc = char(0xDD);
};

struct Channel {
    asio::io_context io;
    asio::serial_port port;
    std::thread thread;
    uint32_t idgen = 0;
    std::unordered_map<uint32_t, py::function> cbs;
    string buffer = string(1024, '\0');
    string last;
    string current;

    virtual ~Channel() {
        io.stop();
        if (thread.joinable()) {
            thread.join();
        }
    }

    Channel(string rawuri) : io(1), port(io) {
        auto uri = Uri::Parse(rawuri);
        if (uri.scheme != "serial") {
            throw Err("Unsupported protocol: {}", uri.scheme);
        }
        using opts = asio::serial_port_base;
        boost::system::error_code ec;
        port.open(uri.path, ec);
        if (ec) {
            throw Err("Could not open: {} => {}", rawuri, ec.message());
        }
        auto baud = opts::baud_rate{uri.GetOr("baud", 57600u)};
        port.set_option(baud, ec);
        if (ec) {
            throw Err("Could set baudrate of {}: {}", baud.value(), ec.message());
        }
        startRead();
        thread = std::thread([this]{
            io.run();
        });
    }
    void startRead() {
        port.async_read_some(asio::buffer(buffer.data(), buffer.size()), [this](auto& ec, auto sz){
            readDone(ec, sz);
        });
    }
    void deescMsg(string& msg) {
        size_t pos = 0;
        bool esc = false;
        fmt::print(stderr, "Original: [{:x}]\n", fmt::join(msg, ","));
        for (auto ch: msg) {
            if (ch == char(SLIP::ESC)) {
                if (esc) {
                    throw Err("Escape byte right after another ESC");
                }
                esc = true;
                continue;
            }
            if (std::exchange(esc, false)) {
                msg[pos++] = [&]{
                    switch (ch) {
                    case SLIP::EscapedEnd: return SLIP::END;
                    case SLIP::EscapedEsc: return SLIP::ESC;
                    default: throw Err("Invalid escaped byte: {}", ch);
                    }
                }();
            } else {
                msg[pos++] = ch;
            }
        }
        handleMsg(string_view{msg.c_str(), pos});
    }

    void handleMsg(string_view msg) {
        fmt::print(stderr, "DeEscaped: [{:x}]\n", fmt::join(msg, ","));
        if (msg.size() < 8) {
            // TODO: msg too small
            return;
        }
        auto p = reinterpret_cast<const uint8_t*>(msg.data());
        auto id = boost::endian::load_little_u32(p);
        auto type = boost::endian::load_little_u16(p + 4);
        auto flags = boost::endian::load_little_u16(p + 6);
        py::gil_scoped_acquire lock;
        if (flags & Request) {
            // TODO: this callback can outlive class
            message(type, py::bytes(msg.substr(8)), [this, id](int type, py::bytes body){
                doSend(type, body, {}, id);
            });
        } else {
            message(type, py::bytes(msg.substr(8)), std::nullopt);
        }
    }

    void readDone(boost::system::error_code const& ec, size_t amount) try {
        if (ec) {
            py::gil_scoped_acquire lock;
            error(ec.message());
            return;
        }
        fmt::print(stderr, "Read done of: {}\n", amount);
        current += last;
        last.clear();
        auto buffview = string_view{buffer.c_str(), amount};
        size_t next;
        while((next = buffview.find_first_of(SLIP::END)) != string::npos) {
            current += buffview.substr(0, next);
            deescMsg(current);
            current.clear();
            if (next == buffview.size()) {
                buffview = {};
            } else {
                buffview = buffview.substr(next + 1);
            }
        }
        last += buffview;
        current = {};
        startRead();
    } catch (std::exception& e) {
        py::gil_scoped_acquire lock;
        error(e.what());
    }

    void send(int type, py::bytes body, std::optional<py::function> ack) {
        doSend(type, body, std::move(ack), idgen++);
    }

    void doSend(int type, py::bytes body, std::optional<py::function> ack, uint32_t id) {
        uint16_t flags = 0;
        auto orig = string_view{body};
        string final;
        final.reserve(orig.size() + 10);
        final.resize(8);
        if (auto& f = ack) {
            flags |= Request;
            auto [iter, ok] = cbs.try_emplace(id, std::move(*f));
            if (!ok) {
                iter->second(std::runtime_error("Timeout"));
                iter->second = std::move(*f);
            }
        }
        auto lid = boost::endian::native_to_little(id);
        auto ltype = boost::endian::native_to_little(uint16_t(type));
        auto lflags = boost::endian::native_to_little(uint16_t(flags));
        memcpy(final.data(), &lid, 4);
        memcpy(final.data() + 4, &ltype, 2);
        memcpy(final.data() + 6, &lflags, 2);
        for (auto ch: orig) {
            switch (ch) {
            case SLIP::END: {
                final += SLIP::ESC;
                final += SLIP::EscapedEnd;
                break;
            }
            case SLIP::ESC: {
                final += SLIP::ESC;
                final += SLIP::EscapedEsc;
                break;
            }
            default: {
                final += ch;
            }
            }
        }
        final += SLIP::END;
        port.async_write_some(asio::buffer(final), [this](auto& ec, size_t written){
            if (ec) {
                py::gil_scoped_acquire lock;
                error(ec.message());
            } else {
                fmt::print(stderr, "Written: {}\n", written);
            }
        });
    }

    virtual void message(int type, py::bytes body, std::optional<py::cpp_function> reply) = 0;
    virtual void error(string msg) {
        py::print("Error: In communication with arduino: ", msg);
    }
};

struct PyChannel : Channel {
    using Channel::Channel;
    void message(int type, py::bytes body, std::optional<py::cpp_function> reply) override {
        PYBIND11_OVERRIDE_PURE(void, Channel, message, type, body, reply);
    }
    void error(string msg) override {
        PYBIND11_OVERRIDE(void, Channel, error, msg);
    }
};

}

using namespace py::literals;

PYBIND11_MODULE(arduino, m) {
    py::class_<arduino::Channel, arduino::PyChannel>(m, "Channel")
        .def(py::init<string>(), "Create comms Channel with device on uri")
        .def("message", &arduino::Channel::message, "Override to handle incoming packets", "type"_a, "body"_a, "reply"_a.none())
        .def("error", &arduino::Channel::error, "Override to handle errors", "msg"_a)
        .def("send", &arduino::Channel::send, "Send packet with optional ack callback", "type"_a, "body"_a, "ack"_a.none());
}
