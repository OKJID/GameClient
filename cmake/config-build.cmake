# Do we want to build extra SDK stuff or just the game binary?
option(RTS_BUILD_CORE_TOOLS "Build core tools" ON)
option(RTS_BUILD_CORE_EXTRAS "Build core extra tools/tests" OFF)
option(RTS_BUILD_ZEROHOUR "Build Zero Hour code." ON)
option(RTS_BUILD_GENERALS "Build Generals code." ON)
option(RTS_BUILD_OPTION_PROFILE "Build code with the \"Profile\" configuration." OFF)
option(RTS_BUILD_OPTION_PROFILE_TRACY "Build code with Tracy profiling enabled." OFF)
option(RTS_BUILD_OPTION_DEBUG "Build code with the \"Debug\" configuration." OFF)
option(RTS_BUILD_OPTION_ASAN "Build code with Address Sanitizer." OFF)
option(RTS_BUILD_OPTION_OPTIMIZED "Enable aggressive MSVC Release optimizations." OFF)
set(RTS_BUILD_OPTION_PGO "OFF" CACHE STRING "MSVC profile-guided optimization mode.")
set_property(CACHE RTS_BUILD_OPTION_PGO PROPERTY STRINGS OFF GENERATE USE)
set(RTS_PGO_FILE "${CMAKE_BINARY_DIR}/genzh.pgd" CACHE FILEPATH "MSVC PGO database path.")
option(RTS_BUILD_OPTION_VC6_FULL_DEBUG "Build VC6 with full debug info." OFF)
option(RTS_BUILD_OPTION_FFMPEG "Enable FFmpeg support" OFF)

if(APPLE)
    set(RTS_BUILD_GENERALS ON CACHE BOOL "" FORCE)
    set(RTS_BUILD_ZEROHOUR_TOOLS OFF CACHE BOOL "" FORCE)
    set(RTS_BUILD_ZEROHOUR_EXTRAS OFF CACHE BOOL "" FORCE)
    set(RTS_BUILD_GENERALS_TOOLS OFF CACHE BOOL "" FORCE)
    set(RTS_BUILD_GENERALS_EXTRAS OFF CACHE BOOL "" FORCE)
    set(RTS_BUILD_CORE_TOOLS OFF CACHE BOOL "" FORCE)
    set(RTS_BUILD_CORE_EXTRAS OFF CACHE BOOL "" FORCE)
endif()

if(NOT RTS_BUILD_ZEROHOUR AND NOT RTS_BUILD_GENERALS)
    set(RTS_BUILD_ZEROHOUR TRUE)
    message("You must select one project to build, building Zero Hour by default.")
endif()

add_feature_info(CoreTools RTS_BUILD_CORE_TOOLS "Build Core Mod Tools")
add_feature_info(CoreExtras RTS_BUILD_CORE_EXTRAS "Build Core Extra Tools/Tests")
add_feature_info(ZeroHourStuff RTS_BUILD_ZEROHOUR "Build Zero Hour code")
add_feature_info(GeneralsStuff RTS_BUILD_GENERALS "Build Generals code")
add_feature_info(ProfileBuild RTS_BUILD_OPTION_PROFILE "Building as a \"Profile\" build")
add_feature_info(DebugBuild RTS_BUILD_OPTION_DEBUG "Building as a \"Debug\" build")
add_feature_info(AddressSanitizer RTS_BUILD_OPTION_ASAN "Building with address sanitizer")
add_feature_info(OptimizedBuild RTS_BUILD_OPTION_OPTIMIZED "Aggressive Release optimization")
add_feature_info(PGO RTS_BUILD_OPTION_PGO "Profile-guided optimization mode")

if(MSVC AND RTS_BUILD_OPTION_OPTIMIZED AND NOT IS_VS6_BUILD)
    target_compile_options(core_config INTERFACE
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/Ob3>
        $<$<CONFIG:Release>:/Oi>
        $<$<CONFIG:Release>:/Ot>
        $<$<CONFIG:Release>:/Gy>
        $<$<CONFIG:Release>:/Gw>
        $<$<CONFIG:Release>:/GL>
        $<$<CONFIG:Release>:/arch:SSE2>
    )
    target_link_options(core_config INTERFACE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

if(MSVC AND NOT IS_VS6_BUILD)
    target_compile_options(core_config INTERFACE
        $<$<CONFIG:Release>:/guard:cf>
    )
    target_link_options(core_config INTERFACE
        $<$<CONFIG:Release>:/DYNAMICBASE>
        $<$<CONFIG:Release>:/NXCOMPAT>
        $<$<CONFIG:Release>:/guard:cf>
    )
endif()

if(NOT "${RTS_BUILD_OPTION_PGO}" MATCHES "^(OFF|GENERATE|USE)$")
    message(FATAL_ERROR "RTS_BUILD_OPTION_PGO must be OFF, GENERATE, or USE")
elseif(MSVC AND NOT "${RTS_BUILD_OPTION_PGO}" STREQUAL "OFF" AND NOT IS_VS6_BUILD)
    target_compile_options(core_config INTERFACE $<$<CONFIG:Release>:/GL>)
    target_link_options(core_config INTERFACE $<$<CONFIG:Release>:/LTCG>)
    if("${RTS_BUILD_OPTION_PGO}" STREQUAL "GENERATE")
        target_link_options(core_config INTERFACE
            $<$<CONFIG:Release>:/GENPROFILE:PGD=${RTS_PGO_FILE}>
        )
    else()
        target_link_options(core_config INTERFACE
            $<$<CONFIG:Release>:/USEPROFILE:PGD=${RTS_PGO_FILE}>
        )
    endif()
endif()
add_feature_info(Vc6FullDebug RTS_BUILD_OPTION_VC6_FULL_DEBUG "Building VC6 with full debug info")
add_feature_info(FFmpegSupport RTS_BUILD_OPTION_FFMPEG "Building with FFmpeg support")

set(RTS_BUILD_OUTPUT_SUFFIX "" CACHE STRING "Suffix appended to output names of installable targets")

if(RTS_BUILD_ZEROHOUR)
    option(RTS_BUILD_ZEROHOUR_TOOLS "Build tools for Zero Hour" ON)
    option(RTS_BUILD_ZEROHOUR_EXTRAS "Build extra tools/tests for Zero Hour" OFF)
    option(RTS_BUILD_ZEROHOUR_DOCS "Build documentation for Zero Hour" OFF)

    add_feature_info(ZeroHourTools RTS_BUILD_ZEROHOUR_TOOLS "Build Zero Hour Mod Tools")
    add_feature_info(ZeroHourExtras RTS_BUILD_ZEROHOUR_EXTRAS "Build Zero Hour Extra Tools/Tests")
    add_feature_info(ZeroHourDocs RTS_BUILD_ZEROHOUR_DOCS "Build Zero Hour Documentation")
endif()

if(RTS_BUILD_GENERALS)
    option(RTS_BUILD_GENERALS_TOOLS "Build tools for Generals" ON)
    option(RTS_BUILD_GENERALS_EXTRAS "Build extra tools/tests for Generals" OFF)
    option(RTS_BUILD_GENERALS_DOCS "Build documentation for Generals" OFF)

    add_feature_info(GeneralsTools RTS_BUILD_GENERALS_TOOLS "Build Generals Mod Tools")
    add_feature_info(GeneralsExtras RTS_BUILD_GENERALS_EXTRAS "Build Generals Extra Tools/Tests")
    add_feature_info(GeneralsDocs RTS_BUILD_GENERALS_DOCS "Build Generals Documentation")
endif()

if(NOT IS_VS6_BUILD)
    # Because we set CMAKE_CXX_STANDARD_REQUIRED and CMAKE_CXX_EXTENSIONS in the compilers.cmake this should be enforced.
    target_compile_features(core_config INTERFACE cxx_std_20)
endif()

if(IS_VS6_BUILD AND RTS_BUILD_OPTION_VC6_FULL_DEBUG)
    target_compile_options(core_config INTERFACE ${RTS_FLAGS} /Zi)
else()
    target_compile_options(core_config INTERFACE ${RTS_FLAGS})
endif()

# This disables a lot of warnings steering developers to use windows only functions/function names.
if(MSVC)
    target_compile_definitions(core_config INTERFACE _CRT_NONSTDC_NO_WARNINGS _CRT_SECURE_NO_WARNINGS $<$<CONFIG:DEBUG>:_DEBUG_CRT>)
endif()

if(UNIX)
    target_compile_definitions(core_config INTERFACE _UNIX)
endif()

if(RTS_BUILD_OPTION_DEBUG)
    target_compile_definitions(core_config INTERFACE RTS_DEBUG WWDEBUG DEBUG)
else()
    target_compile_definitions(core_config INTERFACE RTS_RELEASE NDEBUG)
endif()

if(RTS_BUILD_OPTION_PROFILE)
    target_compile_definitions(core_config INTERFACE RTS_PROFILE_LEGACY)
endif()

# Define a dummy Tracy target when the build option is disabled.
if(RTS_BUILD_OPTION_PROFILE_TRACY)
    include(cmake/tracy.cmake)
else()
    add_library(core_profile_tracy INTERFACE)
endif()

if(APPLE)
    target_include_directories(core_config BEFORE INTERFACE
        ${CMAKE_SOURCE_DIR}/Platform/MacOS/Include
    )
endif()
