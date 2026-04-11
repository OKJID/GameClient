if(APPLE)
    # Configure GameNetworkingSockets build
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    
    # Force FindProtobuf to run in MODULE mode (avoids config mode which lacks protobuf_generate_cpp)
    set(Protobuf_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
    
    # Hint FindProtobuf for Homebrew (Homebrew doesn't include .a normally, so we hint the .dylib)
    if(EXISTS "/opt/homebrew/lib/libprotobuf.dylib")
        set(Protobuf_LIBRARY "/opt/homebrew/lib/libprotobuf.dylib" CACHE FILEPATH "" FORCE)
        set(Protobuf_INCLUDE_DIR "/opt/homebrew/include" CACHE PATH "" FORCE)
        set(Protobuf_PROTOC_EXECUTABLE "/opt/homebrew/bin/protoc" CACHE FILEPATH "" FORCE)
    endif()
    
    # If using modern protobuf where protobuf_generate_cpp is missing but protobuf_generate exists:
    if(NOT COMMAND protobuf_generate_cpp)
        function(protobuf_generate_cpp SRCS HDRS)
            protobuf_generate(LANGUAGE cpp IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/common" OUT_VAR _outvar PROTOS ${ARGN})
            set(${SRCS})
            set(${HDRS})
            foreach(_file ${_outvar})
                if(_file MATCHES "cc$")
                    list(APPEND ${SRCS} ${_file})
                else()
                    list(APPEND ${HDRS} ${_file})
                endif()
            endforeach()
            set(${SRCS} ${${SRCS}} PARENT_SCOPE)
            set(${HDRS} ${${HDRS}} PARENT_SCOPE)
        endfunction()
    endif()
    
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
