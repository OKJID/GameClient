add_executable(g_generals WIN32)

# Use a binary name that doesn't conflict with original game.
if("${CMAKE_SYSTEM}" MATCHES "Windows")
    set_target_properties(g_generals PROPERTIES OUTPUT_NAME "generalsv${RTS_BUILD_OUTPUT_SUFFIX}")
else()
    set_target_properties(g_generals PROPERTIES OUTPUT_NAME "generalsv${RTS_BUILD_OUTPUT_SUFFIX}")
endif()

target_link_libraries(g_generals PRIVATE
    binkstub
    comctl32
    d3d8
    d3dx8
    dinput8
    dxguid
    g_gameengine
    g_gameenginedevice
    gi_always
    imm32
    milesstub
    vfw32
    winmm
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
if (IS_VS6_BUILD)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/BuildVersion.h
"#pragma once

#define VERSION_MAJOR 1
#define VERSION_MINOR 7
#define VERSION_BUILDNUM 601
"
)
else()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/BuildVersion.h
"#pragma once

#define VERSION_MAJOR 1
#define VERSION_MINOR 8
#define VERSION_BUILDNUM 601
"
)
endif()

if(MSVC)
    target_link_options(g_generals PRIVATE "/NODEFAULTLIB:libci.lib")
endif()

target_include_directories(g_generals PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/Code/Libraries
)

target_sources(g_generals PRIVATE
    WinMain.cpp
    WinMain.h
)

# RC files are optional for MinGW builds
# Icon: LoadIcon() handles missing resource gracefully (uses default system icon)
# Manifest: DPI awareness metadata (nice to have but not essential)
# TYPELIB: Broken (0 bytes) and causes windres memory exhaustion errors
if(MSVC)
    # VS2005 and later adds default manifest, we need to turn it off to prevent conflict with custom manifest
    if(NOT IS_VS6_BUILD)
        target_link_options(g_generals PRIVATE
            "/MANIFEST:NO"
        )
    endif()

    target_sources(g_generals PRIVATE
        RTS.RC
    )
endif()

# Strip debug symbols to separate file for MinGW Release builds
# This creates generalsv${RTS_BUILD_OUTPUT_SUFFIX}.exe.debug (similar to MSVC .pdb files)
if(MINGW AND COMMAND add_debug_strip_target)
    add_debug_strip_target(g_generals)
endif()

if(APPLE)
    set_target_properties(g_generals PROPERTIES
        OUTPUT_NAME GeneralsVanilla
        WIN32_EXECUTABLE FALSE
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.playgenerals.vanilla"
        MACOSX_BUNDLE_BUNDLE_NAME "GeneralsVanilla"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.8.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Platform/MacOS/Info.plist.in"
    )

    # Remove Windows-only link libraries, add macOS frameworks
    set_property(TARGET g_generals PROPERTY LINK_LIBRARIES)
    target_link_libraries(g_generals PRIVATE
        g_gameengine
        g_gameenginedevice
        gi_always
        "-framework Metal"
        "-framework MetalKit"
        "-framework AppKit"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework IOKit"
    )

    # Replace WinMain with macOS entry point
    get_target_property(_sources g_generals SOURCES)
    list(REMOVE_ITEM _sources WinMain.cpp WinMain.h)
    set_property(TARGET g_generals PROPERTY SOURCES ${_sources})

    target_sources(g_generals PRIVATE
        ${CMAKE_SOURCE_DIR}/Platform/MacOS/Source/Main/MacOSMain.mm
        ${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib
    )

    set_source_files_properties(${CMAKE_BINARY_DIR}/Platform/MacOS/default.metallib PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # d3d8.h proxies and Win32 stubs
    target_include_directories(g_generals BEFORE PRIVATE
        ${CMAKE_SOURCE_DIR}/Platform/MacOS/Include
    )

    target_link_libraries(g_generals PRIVATE macos_platform)
    macos_platform_add_game_glue(g_generals)
    add_dependencies(g_generals metal_shaders)
endif()
