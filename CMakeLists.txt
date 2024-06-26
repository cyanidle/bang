cmake_minimum_required(VERSION 3.10)
project(bang)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(submodule/lidar-sdk)

include(./cmake/utils/GetCPM.cmake)

CPMAddPackage("gh:cyanidle/describe#1.0.0")
CPMAddPackage("gh:fmtlib/fmt#10.2.0")
CPMAddPackage("gh:pybind/pybind11@2.12.0")

CPMAddPackage(NAME Boost
  VERSION 1.84.0
  URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
  URL_HASH SHA256=2e64e5d79a738d0fa6fb546c6e5c2bd28f88d268a2a080546f74e5ff98f29d0e
  OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_INCLUDE_LIBRARIES asio\\\;endian" # Note the escapes!
)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

add_library(lidar SHARED src/lidar.cpp)
target_include_directories(lidar PRIVATE include)
target_link_libraries(lidar PRIVATE
    Python3::Python pybind11_headers fmt describe
    rplidar-sdk
)
pybind11_extension(lidar)

add_library(arduino SHARED src/arduino.cpp)
target_include_directories(arduino PRIVATE include)
target_link_libraries(arduino PRIVATE
    Python3::Python pybind11_headers fmt describe Boost::asio Boost::endian
)
pybind11_extension(arduino)

if (UNIX)
    target_compile_options(fmt PRIVATE -fPIC)
    target_compile_options(rplidar-sdk PRIVATE -fPIC)
    target_link_options(lidar PRIVATE -Wl,--no-undefined)
endif()

set(MSGS 
    Move Odom Pid ConfigMotor ReadPin
    Test ConfigPinout Echo)

function(Generate name source pyout cout)
    if (pyout)
        list(APPEND kwargs --out_py ${pyout})
    endif()
    if (cout)
        list(APPEND kwargs --out_c ${cout})
    endif()
    execute_process(
        COMMAND
            ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/submodule/ros-iface/generate.py
            --name ${name}
            ${source}
            ${kwargs}
        ERROR_VARIABLE err
    )
    if (err)
        message(FATAL_ERROR "Error Generating: ${err}")
    endif()
endfunction()

foreach(name ${MSGS})
    list(APPEND INIT_PY "from .Msg${name} import *")
    list(APPEND ALL_MSGS "Msg${name}")
    Generate(Msg${name}
        ${CMAKE_SOURCE_DIR}/msg/${name}.msg
        ${CMAKE_SOURCE_DIR}/script/gen/Msg${name}.py
        ${CMAKE_SOURCE_DIR}/firmware/gen/Msg${name}.h)
endforeach()

list(JOIN ALL_MSGS ",\n    " ALL_MSGS)
list(JOIN INIT_PY "\n" INIT_PY)
file(WRITE script/gen/__init__.py "${INIT_PY}\nAllMsgs = [\n    ${ALL_MSGS}\n]")

install(TARGETS lidar arduino DESTINATION bang)
install(DIRECTORY script/gen 
    DESTINATION bang 
    REGEX __pycache__ EXCLUDE)
install(FILES script/bang.py script/odom.py script/__init__.py
    DESTINATION bang 
    PERMISSIONS 
        OWNER_WRITE OWNER_READ OWNER_EXECUTE 
        GROUP_READ GROUP_EXECUTE
        WORLD_READ)
