# Static libraries with the dynamic CRT (/MD), matching CMAKE_MSVC_RUNTIME_LIBRARY.
set(VCPKG_TARGET_ARCHITECTURE x86)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

# Match the engine's _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR: std::mutex built with the
# constexpr constructor (VS 2022 17.10+) crashes in mtx_do_lock on older msvcp140.dll.
set(VCPKG_C_FLAGS "/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")
set(VCPKG_CXX_FLAGS "/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")

# Exclude compiler version from ABI hash so that weekly GitHub runner image
# updates don't invalidate the binary cache. Minor MSVC version bumps do not
# cause ABI incompatibilities for this project.
set(VCPKG_DISABLE_COMPILER_TRACKING ON)

# The gamenetworkingsockets port does not forward static linkage to its
# protobuf lookup, so it would otherwise compile with PROTOBUF_USE_DLLS.
if(PORT STREQUAL "gamenetworkingsockets")
    set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DProtobuf_USE_STATIC_LIBS=ON)
endif()
