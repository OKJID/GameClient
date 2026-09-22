# FindFFMPEG.cmake
# Locates FFmpeg libraries (libavcodec, libavformat, libavutil, libswscale):
# on macOS the static Bink-only build from ffmpeg_bink.cmake, elsewhere pkg-config.
# Sets: FFMPEG_FOUND, FFMPEG_INCLUDE_DIRS, FFMPEG_LIBRARY_DIRS, FFMPEG_LIBRARIES

if(APPLE)
    include(ffmpeg_bink)

    set(FFMPEG_FOUND TRUE)
    set(FFMPEG_INCLUDE_DIRS "${FFMPEG_BINK_INCLUDE_DIR}")
    set(FFMPEG_LIBRARY_DIRS "")
    set(FFMPEG_LIBRARIES ${FFMPEG_BINK_LIBRARIES})
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG
        libavcodec
        libavformat
        libavutil
        libswscale
    )
endif()

if(FFMPEG_FOUND)
    message(STATUS "FFmpeg found: includes=${FFMPEG_INCLUDE_DIRS}, libs=${FFMPEG_LIBRARIES}")
else()
    message(FATAL_ERROR "FFmpeg not found. Install via: brew install ffmpeg")
endif()
