#pragma once

#include <fmt/format.h>
#include <map>
#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <filesystem>
#include "describe/describe.hpp"

namespace fs = std::filesystem;

namespace bang
{

using std::string;
using std::string_view;
using std::vector;
using std::map;
using std::unique_ptr;

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


}
