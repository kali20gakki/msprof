set(PROTOC_CXXFLAGS "-Wno-maybe-uninitialized -Wno-unused-parameter -fPIC -fstack-protector-all -D_FORTIFY_SOURCE=2 -D_GLIBCXX_USE_CXX11_ABI=0 -O2")
set(PROTOC_LDFLAGS "-Wl,-z,relro,-z,now,-z,noexecstack")

ExternalProject_Add(protoc_build
    SOURCE_DIR ${TOP_DIR}/opensource/protobuf
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
        -G ${CMAKE_GENERATOR}
        -Dprotobuf_WITH_ZLIB=OFF
        -Dprotobuf_BUILD_TESTS=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_CXX_FLAGS=${PROTOC_CXXFLAGS}
        -DCMAKE_CXX_LDFLAGS=${PROTOC_LDFLAGS}
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/protoc
        -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        <SOURCE_DIR>/cmake
    BUILD_COMMAND $(MAKE)
    EXCLUDE_FROM_ALL TRUE
)
