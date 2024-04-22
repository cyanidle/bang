cmake_minimum_required(VERSION 3.10)
project(bang)

add_subdirectory(submodule/lidar-sdk)

add_executable(lidar-bridge src/lidar-bridge.cpp)