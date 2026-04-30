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
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.playgenerals.online"
        MACOSX_BUNDLE_BUNDLE_NAME "GeneralsOnlineZH"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.4.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/Platform/MacOS/Info.plist.in"
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

    # EAC/EOS: @rpath for Frameworks/ is configured by assemble_distribution.sh at packaging time
    # (install_name_tool -add_rpath "@executable_path/../Frameworks/" + codesign)

    # EAC/EOS: Auto-download SDK if missing
    set(EOS_SDK_DIR "${CMAKE_SOURCE_DIR}/Dependencies/EOS-SDK-47370208-Release-v1.18.1.2")
    set(EOS_SDK_DYLIB "${EOS_SDK_DIR}/SDK/Bin/libEOSSDK-Mac-Shipping.dylib")
    set(EOS_SDK_ZIP "${CMAKE_SOURCE_DIR}/Dependencies/EOS-SDK-47370208-Release-v1.18.1.2.zip")
    set(EOS_SDK_URL "https://www.playgenerals.online/dev/EOS-SDK-47370208-Release-v1.18.1.2.zip")

    if(NOT EXISTS "${EOS_SDK_DIR}")
        message(STATUS "[EAC] EOS SDK not found, downloading from ${EOS_SDK_URL}...")
        file(DOWNLOAD "${EOS_SDK_URL}" "${EOS_SDK_ZIP}"
            STATUS EOS_DOWNLOAD_STATUS
            SHOW_PROGRESS
        )
        list(GET EOS_DOWNLOAD_STATUS 0 EOS_DOWNLOAD_ERROR)
        if(EOS_DOWNLOAD_ERROR)
            message(WARNING "[EAC] Failed to download EOS SDK (error ${EOS_DOWNLOAD_ERROR}). Anti-cheat will not be available.")
        else()
            message(STATUS "[EAC] Extracting EOS SDK...")
            file(ARCHIVE_EXTRACT INPUT "${EOS_SDK_ZIP}" DESTINATION "${CMAKE_SOURCE_DIR}/Dependencies")
            file(REMOVE "${EOS_SDK_ZIP}")
            message(STATUS "[EAC] EOS SDK ready.")
        endif()
    endif()

    # EAC/EOS: Copy dylib into app bundle Frameworks/
    if(EXISTS "${EOS_SDK_DYLIB}")
        add_custom_command(TARGET z_generals POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:z_generals>/Contents/Frameworks"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${EOS_SDK_DYLIB}" "$<TARGET_BUNDLE_DIR:z_generals>/Contents/Frameworks/"
            COMMENT "Copying EOS SDK dylib to app bundle Frameworks/"
        )
    else()
        message(WARNING "[EAC] libEOSSDK-Mac-Shipping.dylib not found. Anti-cheat will not be available.")
    endif()
endif()
