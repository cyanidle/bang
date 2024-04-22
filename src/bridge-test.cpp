#include <fmt/format.h>
#include <describe/describe.hpp>
#include <argparse/argparse.hpp>
#include "sl_lidar.h"
#include <string_view>
#include <string>

using std::string_view;
using std::string;

template<typename...Args>
void log(string_view fmt, Args const&...a) {
    fmt::println(stderr, fmt, a...);
}

int main(int argc, char *argv[])
{
    argparse::ArgumentParser cli(argv[0]);
    try {
        cli.parse_known_args(argc, argv);
    } catch (std::exception& e) {
        log("Err: {}", e.what());
        std::cerr << cli;
        return 1;
    }
    return 0;
}
