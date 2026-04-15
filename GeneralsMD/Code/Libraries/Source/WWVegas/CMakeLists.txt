# Interface libraries to set common defines and includes, avoid duplication later
add_library(z_wwcommon INTERFACE)

target_link_libraries(z_wwcommon INTERFACE
    core_wwcommon
    stlport
)
if(NOT APPLE)
    target_link_libraries(z_wwcommon INTERFACE
        d3d8lib
        milesstub
    )
endif()

target_include_directories(z_wwcommon INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    WW3D2
)

if(NOT APPLE)
    add_subdirectory(WWAudio)
endif()
add_subdirectory(WW3D2)
if(NOT APPLE)
    add_subdirectory(WWDownload)
endif()

# Helpful interface to bundle the ww modules together.
add_library(z_wwvegas INTERFACE)

target_include_directories(z_wwvegas INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    WW3D2
)

target_link_libraries(z_wwvegas INTERFACE
    core_wwvegas
    z_ww3d2
)
if(NOT APPLE)
    target_link_libraries(z_wwvegas INTERFACE z_wwdownload)
endif()
