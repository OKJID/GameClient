# openssl_static.cmake
# Builds a static OpenSSL for curl and GameNetworkingSockets at the project's macOS deployment target,
# and points find_package(OpenSSL) at it. Must be included before either of them.
# Sets: OPENSSL_STATIC_PREFIX

set(OPENSSL_STATIC_VERSION "3.5.8")
set(OPENSSL_STATIC_SHA256 "a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2")

set(OPENSSL_STATIC_ROOT "${CMAKE_BINARY_DIR}/_deps/openssl_static")
set(OPENSSL_STATIC_BUILD_DIR "${OPENSSL_STATIC_ROOT}/build")
set(OPENSSL_STATIC_PREFIX "${OPENSSL_STATIC_ROOT}/install")
set(OPENSSL_STATIC_STAMP "${OPENSSL_STATIC_PREFIX}/built-${OPENSSL_STATIC_VERSION}-macos${CMAKE_OSX_DEPLOYMENT_TARGET}")

FetchContent_Declare(
    openssl_static
    URL      "https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_STATIC_VERSION}/openssl-${OPENSSL_STATIC_VERSION}.tar.gz"
    URL_HASH "SHA256=${OPENSSL_STATIC_SHA256}"
)
FetchContent_MakeAvailable(openssl_static)

if(NOT EXISTS "${OPENSSL_STATIC_STAMP}")
    message(STATUS "Building OpenSSL ${OPENSSL_STATIC_VERSION} (macOS ${CMAKE_OSX_DEPLOYMENT_TARGET})")

    file(REMOVE_RECURSE "${OPENSSL_STATIC_BUILD_DIR}" "${OPENSSL_STATIC_PREFIX}")
    file(MAKE_DIRECTORY "${OPENSSL_STATIC_BUILD_DIR}")

    execute_process(
        COMMAND perl "${openssl_static_SOURCE_DIR}/Configure"
            "darwin64-${CMAKE_SYSTEM_PROCESSOR}-cc"
            "--prefix=${OPENSSL_STATIC_PREFIX}"
            --libdir=lib
            no-shared
            no-module
            no-legacy
            no-tests
            no-docs
            no-apps
            "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}"
        WORKING_DIRECTORY "${OPENSSL_STATIC_BUILD_DIR}"
        OUTPUT_FILE "${OPENSSL_STATIC_ROOT}/configure.log"
        ERROR_FILE "${OPENSSL_STATIC_ROOT}/configure.log"
        COMMAND_ERROR_IS_FATAL ANY
    )

    cmake_host_system_information(RESULT OPENSSL_STATIC_JOBS QUERY NUMBER_OF_LOGICAL_CORES)

    execute_process(
        COMMAND make "-j${OPENSSL_STATIC_JOBS}" install_sw
        WORKING_DIRECTORY "${OPENSSL_STATIC_BUILD_DIR}"
        OUTPUT_FILE "${OPENSSL_STATIC_ROOT}/build.log"
        ERROR_FILE "${OPENSSL_STATIC_ROOT}/build.log"
        COMMAND_ERROR_IS_FATAL ANY
    )

    file(TOUCH "${OPENSSL_STATIC_STAMP}")
endif()

set(OpenSSL_DIR "${OPENSSL_STATIC_PREFIX}/lib/cmake/OpenSSL" CACHE PATH "" FORCE)
set(OPENSSL_ROOT_DIR "${OPENSSL_STATIC_PREFIX}" CACHE PATH "" FORCE)
set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(OPENSSL_INCLUDE_DIR "${OPENSSL_STATIC_PREFIX}/include" CACHE PATH "" FORCE)
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_STATIC_PREFIX}/lib/libcrypto.a" CACHE FILEPATH "" FORCE)
set(OPENSSL_SSL_LIBRARY "${OPENSSL_STATIC_PREFIX}/lib/libssl.a" CACHE FILEPATH "" FORCE)
