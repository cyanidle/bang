cmake_minimum_required(VERSION 3.10)
project(rplidar-sdk)

set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_STANDARD 11)

FILE(GLOB RPLIDAR_SDK_SRC
  sdk/src/dataunpacker/*.cpp
  sdk/src/dataunpacker/unpacker/*.cpp
  sdk/src/arch/linux/*.cpp
  sdk/src/hal/*.cpp
  sdk/src/*.cpp
)

add_library(rplidar-sdk STATIC ${RPLIDAR_SDK_SRC})
target_include_directories(rplidar-sdk PUBLIC
  sdk/include
  sdk/src
)
