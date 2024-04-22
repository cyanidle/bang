#include "lidar/lidar-bridge.hpp"

int main()
{
    auto serial = lidar::Uri::Parse("serial:/dev/ttyACM0?baud=115200");
}
