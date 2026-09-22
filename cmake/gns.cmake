if(APPLE)
    # Configure GameNetworkingSockets build
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)

    include(protobuf_static)
    set(Protobuf_DIR "${PROTOBUF_STATIC_PREFIX}/lib/cmake/protobuf" CACHE PATH "" FORCE)
    unset(Protobuf_LIBRARY CACHE)
    unset(Protobuf_INCLUDE_DIR CACHE)
    unset(Protobuf_PROTOC_EXECUTABLE CACHE)

    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON CACHE BOOL "" FORCE)

    # Enable WebRTC ICE (implementation=2) instead of Valve native ICE (implementation=1)
    set(USE_STEAMWEBRTC ON CACHE BOOL "" FORCE)
    add_definitions(-DSTEAMNETWORKINGSOCKETS_ENABLE_WEBRTC)

    FetchContent_Declare(
        GameNetworkingSockets
        GIT_REPOSITORY https://github.com/ValveSoftware/GameNetworkingSockets.git
        GIT_TAG        master
    )
    
    FetchContent_MakeAvailable(GameNetworkingSockets)

    # p2p_webrtc.cpp defines CConnectionTransportP2PICE_WebRTC and
    # g_SteamNetworkingSockets_CreateICESessionFunc needed by the ENABLE_WEBRTC code path.
    # GNS upstream CMakeLists doesn't include it in the opensource source list.
    target_sources(GameNetworkingSockets PRIVATE
        "${gamenetworkingsockets_SOURCE_DIR}/src/steamnetworkingsockets/clientlib/steamnetworkingsockets_p2p_webrtc.cpp"
    )

    # libwebrtc-lite.a (cocoa_threading.mm) requires Foundation for NSThread
    find_library(FOUNDATION_FW Foundation)
    if(FOUNDATION_FW)
        target_link_libraries(GameNetworkingSockets PRIVATE ${FOUNDATION_FW})
    endif()
endif()
