include(${CMAKE_CURRENT_LIST_DIR}/protobuf_sym_rename.cmake)

set(SECURITY_COMPILE_OPT "-fPIC -fstack-protector-all -Wl,-z,relro,-z,now,-z,noexecstack -s -D_FORTIFY_SOURCE=2 -O2")
set(PROTOBUF_CXXFLAGS "${SECURITY_COMPILE_OPT} -Wno-maybe-uninitialized -Wno-unused-parameter -D_GLIBCXX_USE_CXX11_ABI=0 -Dgoogle=ascend_private -Wl,-Bsymbolic")
set(PROTOBUF_CXXFLAGS "${PROTOBUF_CXXFLAGS} ${PROTOBUF_SYM_RENAME}")

ExternalProject_Add(ascend_protobuf_shared_build
    SOURCE_DIR ${TOP_DIR}/opensource/protobuf
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
        -G ${CMAKE_GENERATOR}
        -Dprotobuf_WITH_ZLIB=OFF
        -DCMAKE_SKIP_RPATH=TRUE
        -DLIB_PREFIX=ascend_
        -Dprotobuf_BUILD_TESTS=OFF
        -DCMAKE_INSTALL_LIBDIR=lib
        -DBUILD_SHARED_LIBS=ON
        -DCMAKE_CXX_FLAGS=${PROTOBUF_CXXFLAGS}
        -Dprotobuf_BUILD_PROTOC_BINARIES=OFF
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/ascend_protobuf
        <SOURCE_DIR>/cmake
    BUILD_COMMAND $(MAKE)
    EXCLUDE_FROM_ALL TRUE
)







