# ffmpeg_bink.cmake
# Builds a static FFmpeg limited to the Bink demuxer and decoders, at the project's macOS deployment target.
# Sets: FFMPEG_BINK_INCLUDE_DIR, FFMPEG_BINK_LIBRARIES

set(FFMPEG_BINK_VERSION "8.1.2")
set(FFMPEG_BINK_SHA256 "464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c")

set(FFMPEG_BINK_ROOT "${CMAKE_BINARY_DIR}/_deps/ffmpeg_bink")
set(FFMPEG_BINK_BUILD_DIR "${FFMPEG_BINK_ROOT}/build")
set(FFMPEG_BINK_PREFIX "${FFMPEG_BINK_ROOT}/install")
set(FFMPEG_BINK_STAMP "${FFMPEG_BINK_PREFIX}/built-${FFMPEG_BINK_VERSION}-macos${CMAKE_OSX_DEPLOYMENT_TARGET}")
set(FFMPEG_BINK_INCLUDE_DIR "${FFMPEG_BINK_PREFIX}/include")
set(FFMPEG_BINK_LIBRARIES
    "${FFMPEG_BINK_PREFIX}/lib/libavformat.a"
    "${FFMPEG_BINK_PREFIX}/lib/libavcodec.a"
    "${FFMPEG_BINK_PREFIX}/lib/libswscale.a"
    "${FFMPEG_BINK_PREFIX}/lib/libavutil.a"
    "-framework CoreFoundation"
    "-framework CoreVideo"
    "-framework CoreMedia"
)

FetchContent_Declare(
    ffmpeg_bink
    URL      "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_BINK_VERSION}.tar.xz"
    URL_HASH "SHA256=${FFMPEG_BINK_SHA256}"
)
FetchContent_MakeAvailable(ffmpeg_bink)

if(NOT EXISTS "${FFMPEG_BINK_STAMP}")
    message(STATUS "Building FFmpeg ${FFMPEG_BINK_VERSION} for Bink (macOS ${CMAKE_OSX_DEPLOYMENT_TARGET})")

    file(REMOVE_RECURSE "${FFMPEG_BINK_BUILD_DIR}" "${FFMPEG_BINK_PREFIX}")
    file(MAKE_DIRECTORY "${FFMPEG_BINK_BUILD_DIR}")

    set(FFMPEG_BINK_TARGET_FLAG "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")

    execute_process(
        COMMAND "${ffmpeg_bink_SOURCE_DIR}/configure"
            "--prefix=${FFMPEG_BINK_PREFIX}"
            "--arch=${CMAKE_SYSTEM_PROCESSOR}"
            "--cc=${CMAKE_C_COMPILER}"
            --enable-static
            --disable-shared
            --enable-pic
            --disable-everything
            --disable-autodetect
            --disable-programs
            --disable-doc
            --disable-network
            --disable-avdevice
            --disable-avfilter
            --disable-swresample
            --enable-demuxer=bink
            --enable-decoder=bink,binkaudio_rdft,binkaudio_dct
            "--extra-cflags=${FFMPEG_BINK_TARGET_FLAG}"
            "--extra-ldflags=${FFMPEG_BINK_TARGET_FLAG}"
        WORKING_DIRECTORY "${FFMPEG_BINK_BUILD_DIR}"
        OUTPUT_FILE "${FFMPEG_BINK_ROOT}/configure.log"
        ERROR_FILE "${FFMPEG_BINK_ROOT}/configure.log"
        COMMAND_ERROR_IS_FATAL ANY
    )

    cmake_host_system_information(RESULT FFMPEG_BINK_JOBS QUERY NUMBER_OF_LOGICAL_CORES)

    execute_process(
        COMMAND make "-j${FFMPEG_BINK_JOBS}" install
        WORKING_DIRECTORY "${FFMPEG_BINK_BUILD_DIR}"
        OUTPUT_FILE "${FFMPEG_BINK_ROOT}/build.log"
        ERROR_FILE "${FFMPEG_BINK_ROOT}/build.log"
        COMMAND_ERROR_IS_FATAL ANY
    )

    file(TOUCH "${FFMPEG_BINK_STAMP}")
endif()
