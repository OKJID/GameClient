# protobuf_static.cmake
# Builds a static protobuf and a matching protoc for GameNetworkingSockets at the project's macOS deployment target.
# 3.21 is the last line without the abseil dependency, so no abseil dylibs end up in the bundle.
# Sets: PROTOBUF_STATIC_PREFIX

set(PROTOBUF_STATIC_VERSION "3.21.12")
set(PROTOBUF_STATIC_SHA256 "4eab9b524aa5913c6fffb20b2a8abf5ef7f95a80bc0701f3a6dbb4c607f73460")

set(PROTOBUF_STATIC_ROOT "${CMAKE_BINARY_DIR}/_deps/protobuf_static")
set(PROTOBUF_STATIC_BUILD_DIR "${PROTOBUF_STATIC_ROOT}/build")
set(PROTOBUF_STATIC_PREFIX "${PROTOBUF_STATIC_ROOT}/install")
set(PROTOBUF_STATIC_STAMP "${PROTOBUF_STATIC_PREFIX}/built-${PROTOBUF_STATIC_VERSION}-macos${CMAKE_OSX_DEPLOYMENT_TARGET}")

FetchContent_Declare(
    protobuf_static
    URL      "https://github.com/protocolbuffers/protobuf/releases/download/v21.12/protobuf-cpp-${PROTOBUF_STATIC_VERSION}.tar.gz"
    URL_HASH "SHA256=${PROTOBUF_STATIC_SHA256}"
    SOURCE_SUBDIR do-not-add
)
FetchContent_MakeAvailable(protobuf_static)

if(NOT EXISTS "${PROTOBUF_STATIC_STAMP}")
    message(STATUS "Building protobuf ${PROTOBUF_STATIC_VERSION} (macOS ${CMAKE_OSX_DEPLOYMENT_TARGET})")

    file(REMOVE_RECURSE "${PROTOBUF_STATIC_BUILD_DIR}" "${PROTOBUF_STATIC_PREFIX}")

    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            -S "${protobuf_static_SOURCE_DIR}"
            -B "${PROTOBUF_STATIC_BUILD_DIR}"
            -G "${CMAKE_GENERATOR}"
            -DCMAKE_BUILD_TYPE=Release
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
            "-DCMAKE_INSTALL_PREFIX=${PROTOBUF_STATIC_PREFIX}"
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -Dprotobuf_BUILD_TESTS=OFF
            -Dprotobuf_BUILD_SHARED_LIBS=OFF
            -Dprotobuf_WITH_ZLIB=OFF
        OUTPUT_FILE "${PROTOBUF_STATIC_ROOT}/configure.log"
        ERROR_FILE "${PROTOBUF_STATIC_ROOT}/configure.log"
        COMMAND_ERROR_IS_FATAL ANY
    )

    execute_process(
        COMMAND "${CMAKE_COMMAND}" --build "${PROTOBUF_STATIC_BUILD_DIR}" --target install
        OUTPUT_FILE "${PROTOBUF_STATIC_ROOT}/build.log"
        ERROR_FILE "${PROTOBUF_STATIC_ROOT}/build.log"
        COMMAND_ERROR_IS_FATAL ANY
    )

    file(TOUCH "${PROTOBUF_STATIC_STAMP}")
endif()
