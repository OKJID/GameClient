# FindFFMPEG.cmake
# Uses pkg-config to locate FFmpeg libraries (libavcodec, libavformat, libavutil, libswscale).
# Sets: FFMPEG_FOUND, FFMPEG_INCLUDE_DIRS, FFMPEG_LIBRARY_DIRS, FFMPEG_LIBRARIES

find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG
    libavcodec
    libavformat
    libavutil
    libswscale
)

if(FFMPEG_FOUND)
    message(STATUS "FFmpeg found: includes=${FFMPEG_INCLUDE_DIRS}, libs=${FFMPEG_LIBRARIES}")
else()
    message(FATAL_ERROR "FFmpeg not found. Install via: brew install ffmpeg")
endif()
