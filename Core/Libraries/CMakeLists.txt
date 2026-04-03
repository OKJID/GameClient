# WW common libraries decended from earliest C&C games
add_subdirectory(Source/WWVegas)

# profiling library
if(NOT APPLE)
    add_subdirectory(Source/profile)
endif()

# debugging library
if(NOT APPLE)
    add_subdirectory(Source/debug)
endif()

add_subdirectory(Source/EABrowserDispatch)
add_subdirectory(Source/EABrowserEngine)
add_subdirectory(Source/Compression)
