cmake_minimum_required(VERSION 3.10)
project(bang)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(submodule/lidar-sdk)

# CPM
file(DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494
)

include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:cyanidle/describe#1.0.0")
CPMAddPackage("gh:fmtlib/fmt#10.2.0")
CPMAddPackage("gh:pybind/pybind11@2.12.0")

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_library(lidarbridge SHARED src/lidar-bridge.cpp)
target_link_libraries(lidarbridge PRIVATE
    Python3::Python
    pybind11_headers
    fmt
    describe
    rplidar-sdk)

pybind11_extension(lidarbridge)

if (UNIX)
    target_compile_options(fmt PRIVATE -fPIC)
    target_compile_options(rplidar-sdk PRIVATE -fPIC)
    target_link_options(lidarbridge PRIVATE -Wl,--no-undefined)
endif()

function(Generate source out)
    execute_process(
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/submodule/ros-iface/generate.py
            ${source}
            -o ${out}
        ERROR_VARIABLE err
    )
    if (err)
        message(FATAL_ERROR "Error Generating: ${err}")
    endif()
endfunction()

Generate(${CMAKE_SOURCE_DIR}/msg/Move.msg ${CMAKE_CURRENT_BINARY_DIR}/Gen/Move)
Generate(${CMAKE_SOURCE_DIR}/msg/Servo.msg ${CMAKE_CURRENT_BINARY_DIR}/Gen/Servo)
Generate(${CMAKE_SOURCE_DIR}/msg/Odom.msg ${CMAKE_CURRENT_BINARY_DIR}/Gen/Odom)
Generate(${CMAKE_SOURCE_DIR}/msg/Pid.msg ${CMAKE_CURRENT_BINARY_DIR}/Gen/Pid)
