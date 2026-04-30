cmake_minimum_required(VERSION 3.21)
project(eac_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# EOS SDK paths
set(EOS_SDK_DIR "${CMAKE_SOURCE_DIR}/../../../Dependencies/EOS-SDK-47370208-Release-v1.18.1.2/SDK")
set(EOS_INCLUDE_DIR "${EOS_SDK_DIR}/Include")
set(EOS_LIB_DIR "${EOS_SDK_DIR}/Bin")
set(EOS_DYLIB "${EOS_LIB_DIR}/libEOSSDK-Mac-Shipping.dylib")

# Plugin shared library
add_library(easyanticheat SHARED eac_plugin.cpp)

target_include_directories(easyanticheat PRIVATE "${EOS_INCLUDE_DIR}")
target_link_libraries(easyanticheat PRIVATE "${EOS_DYLIB}")

# Export all C symbols
set_target_properties(easyanticheat PROPERTIES
    OUTPUT_NAME "easyanticheat"
    PREFIX ""
    SUFFIX ".dylib"
    MACOSX_RPATH TRUE
    INSTALL_RPATH "@loader_path"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Ensure fat binary (arm64 + x86_64) if needed
if(CMAKE_OSX_ARCHITECTURES)
    set_target_properties(easyanticheat PROPERTIES OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
endif()
