add_executable(z_generals WIN32)

# Use a binary name that doesn't conflict with original game.
if("${CMAKE_SYSTEM}" MATCHES "Windows")
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME "GeneralsOnlineZH${RTS_BUILD_OUTPUT_SUFFIX}")
else()
    set_target_properties(z_generals PROPERTIES OUTPUT_NAME "GeneralsOnlineZH${RTS_BUILD_OUTPUT_SUFFIX}")
endif()

target_link_libraries(z_generals PRIVATE
    binkstub
    comctl32
    core_debug
    core_profile_legacy
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
    VCRuntimeCheck.cpp
    WinMain.cpp
    WinMain.h
)

# Stamp the file version of the runtime this toolset ships, the minimum VCRuntimeCheck.cpp accepts
if(MSVC AND NOT IS_VS6_BUILD)
    # cl.exe lives in VC/Tools/MSVC/<ver>/bin/Host<arch>/<arch>
    get_filename_component(_go_vc_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(_go_vc_dir "${_go_vc_dir}/../../../../../.." ABSOLUTE)
    set(_go_redist_version_file "${_go_vc_dir}/Auxiliary/Build/Microsoft.VCRedistVersion.default.txt")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_go_redist_arch x64)
    else()
        set(_go_redist_arch x86)
    endif()

    set(_go_msvcp "")
    if(EXISTS "${_go_redist_version_file}")
        file(STRINGS "${_go_redist_version_file}" _go_redist_dir_version LIMIT_COUNT 1)
        string(STRIP "${_go_redist_dir_version}" _go_redist_dir_version)
        file(GLOB _go_msvcp "${_go_vc_dir}/Redist/MSVC/${_go_redist_dir_version}/${_go_redist_arch}/Microsoft.VC*.CRT/msvcp140.dll")
    endif()

    set(_go_msvcp_version "")
    if(_go_msvcp)
        list(GET _go_msvcp 0 _go_msvcp)
        execute_process(
            COMMAND powershell -NoProfile -NonInteractive -Command "(Get-Item -LiteralPath '${_go_msvcp}').VersionInfo.FileVersion"
            OUTPUT_VARIABLE _go_msvcp_version
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
    endif()

    if(_go_msvcp_version MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        message(STATUS "VC++ runtime check requires ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3} or newer")
        set_source_files_properties(VCRuntimeCheck.cpp PROPERTIES COMPILE_DEFINITIONS
            "GO_VCREDIST_MAJOR=${CMAKE_MATCH_1};GO_VCREDIST_MINOR=${CMAKE_MATCH_2};GO_VCREDIST_BUILD=${CMAKE_MATCH_3}")
    else()
        message(WARNING "Could not read the toolset's msvcp140.dll version, the VC++ runtime check falls back to the compiler version")
    endif()

    # New STL features live in satellite DLLs older redistributables lack, and a missing import stops the
    # loader before any game code runs. Delay-load them so VCRuntimeCheck.cpp can detect and install instead.
    # LNK4199 is the harmless "no imports found" warning for satellites the exe doesn't use (yet).
    target_link_options(z_generals PRIVATE
        "/DELAYLOAD:msvcp140_atomic_wait.dll"
        "/DELAYLOAD:msvcp140_1.dll"
        "/DELAYLOAD:msvcp140_2.dll"
        "/DELAYLOAD:msvcp140_codecvt_ids.dll"
        "/IGNORE:4199"
    )
    target_link_libraries(z_generals PRIVATE delayimp)
endif()

if(WIN32)
	add_custom_command(TARGET z_generals POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/../GameEngine/Source/GameNetwork/GeneralsOnline/Vendor/DiscordRPC/discord-rpc.dll"
			"$<TARGET_FILE_DIR:z_generals>/discord-rpc.dll"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/../GameEngine/Source/GameNetwork/GeneralsOnline/Vendor/DiscordRPC/LICENSE.txt"
			"$<TARGET_FILE_DIR:z_generals>/discord-rpc-LICENSE.txt"
	)

	if(EXISTS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/sentry-native/crashpad_handler.exe")
		# sentry's crashpad backend is an out-of-process watcher: it needs its
		# own executable next to the game, at the default handler path sentry
		# looks for (see sentry_options_set_handler_path in sentry.h).
		add_custom_command(TARGET z_generals POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/sentry-native/crashpad_handler.exe"
				"$<TARGET_FILE_DIR:z_generals>/crashpad_handler.exe"
		)
	endif()
endif()

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
    macos_platform_add_game_glue(z_generals)
    add_dependencies(z_generals metal_shaders)

    add_custom_command(TARGET z_generals POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/sentry-native/crashpad_handler"
            "$<TARGET_FILE_DIR:z_generals>/crashpad_handler"
    )

    # EAC/EOS: @rpath must include Frameworks/ so dlopen(easyanticheat.dylib)
    # can resolve its @rpath/libEOSSDK-Mac-Shipping.dylib dependency at runtime.
    set_target_properties(z_generals PROPERTIES
        BUILD_RPATH "@executable_path/../Frameworks;${CMAKE_BINARY_DIR}/eac_plugin"
    )

    # EAC Plugin (private mirror of GeneralsOnlineDevelopmentTeam/AntiCheatPlugin_EasyAntiCheat).
    # Always built so the port keeps compiling; bundled only with all EOS credentials.
    if(EXISTS "${CMAKE_SOURCE_DIR}/Dependencies/eac_plugin/CMakeLists.txt")
        foreach(cred EAC_EOS_PRODUCT_ID EAC_EOS_SANDBOX_ID EAC_EOS_DEPLOYMENT_ID EAC_EOS_CLIENT_ID EAC_EOS_CLIENT_SECRET)
            if(DEFINED ENV{${cred}} AND "${${cred}}" STREQUAL "")
                set(${cred} "$ENV{${cred}}" CACHE STRING "EOS credential: ${cred}" FORCE)
            endif()
        endforeach()

        add_subdirectory("${CMAKE_SOURCE_DIR}/Dependencies/eac_plugin" "${CMAKE_BINARY_DIR}/eac_plugin")
        add_dependencies(z_generals plugin_eac)

        set(EAC_EOS_CREDENTIALS_COMPLETE TRUE)
        foreach(cred EAC_EOS_PRODUCT_ID EAC_EOS_SANDBOX_ID EAC_EOS_DEPLOYMENT_ID EAC_EOS_CLIENT_ID EAC_EOS_CLIENT_SECRET)
            if("${${cred}}" STREQUAL "")
                set(EAC_EOS_CREDENTIALS_COMPLETE FALSE)
            endif()
        endforeach()

        if(EAC_EOS_CREDENTIALS_COMPLETE)
            add_custom_command(TARGET z_generals POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:z_generals>/Contents/Frameworks"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/Dependencies/eac_plugin/EOS/Bin/libEOSSDK-Mac-Shipping.dylib" "$<TARGET_BUNDLE_DIR:z_generals>/Contents/Frameworks/"
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:z_generals>/plugins/easyanticheat"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:plugin_eac>" "$<TARGET_FILE_DIR:z_generals>/plugins/easyanticheat/"
            )
        else()
            add_custom_command(TARGET z_generals POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:z_generals>/plugins/easyanticheat"
                COMMAND ${CMAKE_COMMAND} -E rm -f "$<TARGET_BUNDLE_DIR:z_generals>/Contents/Frameworks/libEOSSDK-Mac-Shipping.dylib"
            )
            message(STATUS "EAC Plugin: EOS credentials incomplete, plugin is built but not bundled.")
        endif()
    else()
        message(STATUS "EAC Plugin source not found (private repo). Skipping plugin build.")
    endif()
endif()
