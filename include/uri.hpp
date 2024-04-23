#pragma once
#include "common.hpp"
#include <charconv>

namespace bang 
{

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

    template<typename T>
    T GetOr(string_view k, T adef) {
        return bang::GetOr(params, k, adef);
    }
};


}
