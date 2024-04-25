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
    Ack = Request,
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
    string rawbuffer = string(1024, '\0');
    string buffer;
    bool esc = false;
    bool err = false;

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
        auto baud = opts::baud_rate{uri.GetOr("baud", 115200u)};
        port.set_option(baud, ec);
        if (ec) {
            throw Err("Could set baudrate of {}: {}", baud.value(), ec.message());
        }
        startRead();
        thread = std::thread([this]{
            boost::system::error_code ec;
            io.run(ec);
            if (ec) {
                py::gil_scoped_acquire lock;
                error("Could not run io: " + ec.message());
            }
        });
    }
    void startRead() {
        port.async_read_some(asio::buffer(rawbuffer.data(), rawbuffer.size()), [this](auto& ec, auto sz){
            readDone(ec, sz);
            startRead();
        });
    }

    void handleMsg(string_view msg) {
        if (msg.size() < 8) {
            throw Err("Msg is too small: {} < 8", msg.size());
        }
        auto p = reinterpret_cast<const uint8_t*>(msg.data());
        auto id = boost::endian::load_little_u32(p);
        auto type = boost::endian::load_little_u16(p + 4);
        auto flags = boost::endian::load_little_u16(p + 6);
        if (!PyGILState_Check()) {
            py::gil_scoped_acquire lock;
            if (flags & Ack) {
                if (msg.size() != 8) {
                    throw Err("Received ACK which is too long: {}", msg.size());
                }
                if (auto it = cbs.find(id); it != cbs.end()) {
                    it->second();
                    cbs.erase(it);
                }
            } else {
                message(type, py::bytes(msg.substr(8)));
            }
        }
    }

    void readDone(boost::system::error_code const& ec, size_t amount) try {
        if (ec) {
            if (!PyGILState_Check()) {
                py::gil_scoped_acquire lock;
                error(ec.message());
                return;
            }
        }
        auto recv = string_view{rawbuffer.data(), amount};
        for (auto ch: recv) {
            if (err && ch == SLIP::END) {
                err = false;
                buffer.clear();
                continue;
            }
            if (buffer.size() > 20 * 1024) {
                err = true;
                continue;
            }
            if (std::exchange(esc, false)) {
                switch (ch) {
                case SLIP::EscapedEnd: {
                    buffer += SLIP::END;
                    continue;
                }
                case SLIP::EscapedEsc: {
                    buffer += SLIP::ESC;
                    continue;
                }
                default: {
                    err = true;
                    continue;
                }
                }
            } else if (ch == SLIP::ESC) {
                if (std::exchange(esc, true)) {
                    err = true;
                    continue;
                }
            } else if (ch == SLIP::END) {
                handleMsg(buffer);
                buffer.clear();
            } else {
                buffer += ch;
            }
        }
    } catch (std::exception& e) {
        startRead();
        if (!PyGILState_Check()) {
            py::gil_scoped_acquire lock;
            error(e.what());
        }
    }

    void send(int type, py::bytes body) {
        doSend(type, body, std::nullopt, idgen++);
    }

    void send_with_ack(int type, py::bytes body, py::function ack) {
        doSend(type, body, std::move(ack), idgen++);
    }

    void doSend(int type, py::bytes body, std::optional<py::function> ack, uint32_t id) {
        uint16_t flags = 0;
        auto orig = string_view{body};
        auto final = std::make_shared<string>();
        final->reserve(orig.size() + 10);
        final->resize(8);
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
        memcpy(final->data(), &lid, 4);
        memcpy(final->data() + 4, &ltype, 2);
        memcpy(final->data() + 6, &lflags, 2);
        for (auto ch: orig) {
            switch (ch) {
            case SLIP::END: {
                *final += SLIP::ESC;
                *final += SLIP::EscapedEnd;
                break;
            }
            case SLIP::ESC: {
                *final += SLIP::ESC;
                *final += SLIP::EscapedEsc;
                break;
            }
            default: {
                *final += ch;
            }
            }
        }
        *final += SLIP::END;
        port.async_write_some(asio::buffer(*final), [final, this](auto& ec, size_t){
            if (ec) {
                py::gil_scoped_acquire lock;
                error(ec.message());
            }
        });
    }

    virtual void message(int type, py::bytes body) = 0;
    virtual void error(string msg) {
        py::print("[!] Arduino Error: ", msg);
    }
    virtual void log(string msg) {
        py::print("[+] Arduino: ", msg);
    }
};

struct PyChannel : Channel {
    using Channel::Channel;
    void message(int type, py::bytes body) override {
        PYBIND11_OVERRIDE_PURE(void, Channel, message, type, body);
    }
    void error(string msg) override {
        PYBIND11_OVERRIDE(void, Channel, error, msg);
    }
    void log(string msg) override {
        PYBIND11_OVERRIDE(void, Channel, log, msg);
    }
    ~PyChannel() {
        if (!PyGILState_Check()) {
            py::gil_scoped_acquire lock;
            cbs.clear();
        } else {
            cbs.clear();
        }
    }
};

}

using namespace py::literals;

PYBIND11_MODULE(arduino, m) {
    py::class_<arduino::Channel, arduino::PyChannel>(m, "Channel")
        .def(py::init<string>(), "Create comms Channel with device on uri")
        .def("message", &arduino::Channel::message,
             "Override to handle incoming packets",
             "type"_a, "body"_a)
        .def("error", &arduino::Channel::error,
             "Override to handle errors",
             "msg"_a)
        .def("log", &arduino::Channel::log,
             "Override to handle logs",
             "msg"_a)
        .def("send", &arduino::Channel::send,
             "Send packet",
             "type"_a, "body"_a)
        .def("send_with_ack", &arduino::Channel::send_with_ack,
             "Send packet with ack callback",
             "type"_a, "body"_a, "ack"_a);
}
