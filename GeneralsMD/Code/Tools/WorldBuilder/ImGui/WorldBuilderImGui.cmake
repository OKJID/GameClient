add_library(z_worldbuilder_probe OBJECT EXCLUDE_FROM_ALL)

target_sources(z_worldbuilder_probe PRIVATE ${WORLDBUILDER_SRC})

target_include_directories(z_worldbuilder_probe BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGui/compat
)

target_include_directories(z_worldbuilder_probe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    include
    res
)

target_compile_options(z_worldbuilder_probe PRIVATE -ferror-limit=0)

target_precompile_headers(z_worldbuilder_probe PRIVATE
    [["Utility/CppMacros.h"]]
    [["WWLib/always.h"]]
    [["Common/STLTypedefs.h"]]
    [["StdAfx.h"]]
    [["WWLib/WWCommon.h"]]
)

target_link_libraries(z_worldbuilder_probe PRIVATE
    core_debug
    core_profile_legacy
    z_gameengine
    z_gameenginedevice
    zi_always
)
