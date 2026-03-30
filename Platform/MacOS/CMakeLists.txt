# Platform/MacOS — Metal backend, Cocoa integration, platform stubs
# This library provides the macOS platform layer that replaces Win32Device components.

# ── Metal rendering backend (DX8 → Metal) ──
set(METAL_SRC
    Source/Metal/MetalDevice8.mm
    Source/Metal/MetalInterface8.mm
    Source/Metal/MetalSurface8.mm
    Source/Metal/MetalTexture8.mm
    Source/Metal/MetalVertexBuffer8.mm
    Source/Metal/MetalIndexBuffer8.mm
    Source/Metal/dx8wrapper_metal.mm
    Source/Metal/MetalBridgeMappings.h
    Source/Metal/MetalDevice8.h
    Source/Metal/MetalFormatConvert.h
    Source/Metal/MetalInterface8.h
    Source/Metal/MetalIndexBuffer8.h
    Source/Metal/MetalSurface8.h
    Source/Metal/MetalTexture8.h
    Source/Metal/MetalTextureCapture.h
    Source/Metal/MetalVertexBuffer8.h
)

# ── Main application (game engine, entry handled by GeneralsMD/Code/Main) ──
set(MAIN_SRC
    Source/Main/MacOSGameEngine.mm
    Source/Main/MacOSGameEngine.h
)

# ── Audio ──
set(AUDIO_SRC
    Source/Audio/MacOSAudioManager.cpp
    Source/Audio/MacOSAudioManager.h
)

# ── Input ──
set(INPUT_SRC
    Source/Input/MacOSKeyboard.mm
    Source/Input/MacOSKeyboard.h
    Source/Input/MacOSMouse.mm
    Source/Input/MacOSMouse.h
)

# ── Combine all sources ──
add_library(macos_platform STATIC
    ${METAL_SRC}
    ${MAIN_SRC}
    ${AUDIO_SRC}
    ${INPUT_SRC}
)

# ── Include directories ──
target_include_directories(macos_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Metal
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Main
)

target_include_directories(macos_platform PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWLib
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWDebug
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWMath
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWSaveLoad
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WW3D2
    ${CMAKE_SOURCE_DIR}/Core/Libraries/Source/WWVegas/WWAudio
    ${CMAKE_SOURCE_DIR}/Core/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/Core/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include/Precompiled
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngine/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/GameEngineDevice/Include
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas
    ${CMAKE_SOURCE_DIR}/GeneralsMD/Code/Libraries/Source/WWVegas/WW3D2
)

# ── macOS frameworks ──
find_library(METAL_FRAMEWORK Metal REQUIRED)
find_library(METALKIT_FRAMEWORK MetalKit REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
find_library(APPKIT_FRAMEWORK AppKit REQUIRED)
find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
find_library(AVFOUNDATION_FRAMEWORK AVFoundation REQUIRED)

target_link_libraries(macos_platform PUBLIC
    ${METAL_FRAMEWORK}
    ${METALKIT_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${COCOA_FRAMEWORK}
    ${APPKIT_FRAMEWORK}
    ${IOKIT_FRAMEWORK}
    ${AUDIOTOOLBOX_FRAMEWORK}
    ${AVFOUNDATION_FRAMEWORK}
)

# ── Project dependencies ──
target_link_libraries(macos_platform PUBLIC
    core_config
    corei_always
    corei_gameengine_include
    resources
)

target_link_libraries(macos_platform PRIVATE
    zi_always
    zi_gameengine_include
)

# ── Compile Metal shaders ──
execute_process(
    COMMAND xcrun -sdk macosx metal --version
    RESULT_VARIABLE METAL_CHECK_RESULT
    OUTPUT_QUIET ERROR_QUIET
)
if(NOT METAL_CHECK_RESULT EQUAL 0)
    message(STATUS "Metal Toolchain not found — downloading automatically...")
    execute_process(
        COMMAND xcodebuild -downloadComponent MetalToolchain
        RESULT_VARIABLE METAL_DL_RESULT
    )
    if(NOT METAL_DL_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to download Metal Toolchain. Please run manually:\n  xcodebuild -downloadComponent MetalToolchain")
    endif()
    message(STATUS "Metal Toolchain installed successfully")
endif()

set(METAL_SHADER_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Source/Metal/MacOSShaders.metal)
set(METAL_AIR_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/MacOSShaders.air)
set(METAL_LIB_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/default.metallib)

add_custom_command(
    OUTPUT ${METAL_AIR_OUTPUT}
    COMMAND xcrun -sdk macosx metal -c ${METAL_SHADER_SOURCE} -o ${METAL_AIR_OUTPUT}
    DEPENDS ${METAL_SHADER_SOURCE}
    COMMENT "Compiling Metal shaders"
)

add_custom_command(
    OUTPUT ${METAL_LIB_OUTPUT}
    COMMAND xcrun -sdk macosx metallib ${METAL_AIR_OUTPUT} -o ${METAL_LIB_OUTPUT}
    DEPENDS ${METAL_AIR_OUTPUT}
    COMMENT "Linking Metal shader library"
)

add_custom_target(metal_shaders ALL DEPENDS ${METAL_LIB_OUTPUT})
add_dependencies(macos_platform metal_shaders)

# ── ObjC++ settings ──
target_compile_options(macos_platform PRIVATE
    $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>
)

target_precompile_headers(macos_platform PRIVATE
    [["Utility/CppMacros.h"]]
)
