# Fetch Dear ImGui (docking branch) for the macOS WorldBuilder.

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        9acdfbf46810c0c74ab281ce04122c4149ae8bd1 # v1.92.9b-docking
)

FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
)

target_compile_definitions(imgui PUBLIC
    IMGUI_USE_BGRA_PACKED_COLOR
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)

if(APPLE)
    target_sources(imgui PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_osx.mm)
    set_source_files_properties(${imgui_SOURCE_DIR}/backends/imgui_impl_osx.mm PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc"
    )
    target_link_libraries(imgui PUBLIC "-framework AppKit" "-framework GameController")
endif()
