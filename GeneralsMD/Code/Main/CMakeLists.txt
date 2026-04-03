add_executable(z_generals WIN32)

# Use a binary name that doesn't conflict with original game.
if("${CMAKE_SYSTEM}" MATCHES "Windows")
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME GeneralsOnlineZH)
else()
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME GeneralsOnlineZH)
endif()

target_link_libraries(z_generals PRIVATE
    binkstub
    comctl32
    core_debug
    core_profile
    d3d8
    d3dx8
    dinput8
    dxguid
    imm32
    milesstub
    vfw32
    winmm
    z_gameengine
    z_gameenginedevice
    zi_always
)

# TODO Originally referred to build host and user, replace with git info perhaps?
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/GeneratedVersion.h
"#pragma once

#define VERSION_LOCALBUILDNUM 0
#define VERSION_BUILDUSER \"\"
#define VERSION_BUILDLOC \"\"
"
)

# Based on original binary values for these variables.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/BuildVersion.h
"#pragma once

#define VERSION_MAJOR 1
#define VERSION_MINOR 4
#define VERSION_BUILDNUM 601
"
)

if(MSVC)
    target_link_options(z_generals PRIVATE "/NODEFAULTLIB:libci.lib")
endif()

target_include_directories(z_generals PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_sources(z_generals PRIVATE
    WinMain.cpp
    WinMain.h
)

# RC files optional for MinGW builds
# Icon: LoadIcon() handles missing resource gracefully (uses default system icon)
# Manifest: DPI awareness metadata (nice to have but not essential)
if(MSVC)
    # VS2005 and later adds default manifest, we need to turn it off to prevent conflict with custom manifest
    if(NOT IS_VS6_BUILD)
        target_link_options(z_generals PRIVATE
            "/MANIFEST:NO"
        )
    endif()

    target_sources(z_generals PRIVATE
        RTS.RC
    )
endif()

# Strip debug symbols to separate file for MinGW Release builds
# This creates generalszh${RTS_BUILD_OUTPUT_SUFFIX}.exe.debug (similar to MSVC .pdb files)
if(MINGW AND COMMAND add_debug_strip_target)
    add_debug_strip_target(z_generals)
endif()

if(APPLE)
    set_target_properties(z_generals PROPERTIES
        WIN32_EXECUTABLE FALSE
        MACOSX_BUNDLE TRUE
    )

    # Remove Windows-only link libraries, add macOS frameworks
    set_property(TARGET z_generals PROPERTY LINK_LIBRARIES)
    target_link_libraries(z_generals PRIVATE
        z_gameengine
        z_gameenginedevice
        zi_always
        "-framework Metal"
        "-framework MetalKit"
        "-framework AppKit"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework IOKit"
    )

    # Replace WinMain with macOS entry point
    get_target_property(_sources z_generals SOURCES)
    list(REMOVE_ITEM _sources WinMain.cpp WinMain.h)
    set_property(TARGET z_generals PROPERTY SOURCES ${_sources})

    target_sources(z_generals PRIVATE
        ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/Main/MacOSMain.mm
        ${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib
    )

    set_source_files_properties(${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # d3d8.h proxies and Win32 stubs
    target_include_directories(z_generals BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/Platform/MacOS/Include
    )

    target_link_libraries(z_generals PRIVATE macos_platform)
    add_dependencies(z_generals metal_shaders)
endif()
