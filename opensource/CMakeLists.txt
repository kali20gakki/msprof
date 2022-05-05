########################## ascend-protobuf ##################################
include(${TOP_DIR}/opensource/protobuf_sym_rename.cmake)
include(ExternalProject)
set(SECURITY_COMPILE_OPT "-fPIC -fstack-protector-all -Wl,-z,relro,-z,now,-z,noexecstack -s -D_FORTIFY_SOURCE=2 -O2")
set(PROTOBUF_CXXFLAGS "${SECURITY_COMPILE_OPT} -Wno-maybe-uninitialized -Wno-unused-parameter -D_GLIBCXX_USE_CXX11_ABI=0 -Dgoogle=ascend_private -Wl,-Bsymbolic")
set(PROTOBUF_CXXFLAGS "${PROTOBUF_CXXFLAGS} ${PROTOBUF_SYM_RENAME}")

ExternalProject_Add(ascend_protobuf_static
    SOURCE_DIR ${TOP_DIR}/opensource/protobuf
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
        -G ${CMAKE_GENERATOR}
        -Dprotobuf_WITH_ZLIB=OFF
        -DCMAKE_SKIP_RPATH=TRUE
        -DLIB_PREFIX=ascend_
        -Dprotobuf_BUILD_TESTS=OFF
        -DBUILD_SHARED_LIBS=OFF
        -DCMAKE_CXX_FLAGS=${PROTOBUF_CXXFLAGS}
        -DCMAKE_INSTALL_PREFIX=${TOP_DIR}/out
        <SOURCE_DIR>/cmake
    BUILD_COMMAND  make -j64
)

ExternalProject_Add(ascend_protobuf_shared
    SOURCE_DIR ${TOP_DIR}/opensource/protobuf
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
        -G ${CMAKE_GENERATOR}
        -Dprotobuf_WITH_ZLIB=OFF
        -DCMAKE_SKIP_RPATH=TRUE
        -DLIB_PREFIX=ascend_
        -Dprotobuf_BUILD_TESTS=OFF
        -DBUILD_SHARED_LIBS=ON
        -DCMAKE_CXX_FLAGS=${PROTOBUF_CXXFLAGS}
        -Dprotobuf_BUILD_PROTOC_BINARIES=OFF
        -DCMAKE_INSTALL_PREFIX=${TOP_DIR}/out
        <SOURCE_DIR>/cmake
    BUILD_COMMAND  make -j64
)
########################## securec ###################################
set(SECUREC_DIR ${TOP_DIR}/opensource/securec)
file(GLOB_RECURSE SECUREC_Src ${SECUREC_DIR}/src/*.c)
set(SECUREC_Inc ${SECUREC_DIR}/include)

add_library(securec_share SHARED
    ${SECUREC_Src}
)

target_include_directories(securec_share PRIVATE
    ${SECUREC_Inc}
)

target_compile_options(securec_share PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)
 
set_target_properties(securec_share
    PROPERTIES
    OUTPUT_NAME c_sec
    LIBRARY_OUTPUT_DIRECTORY ${TOP_DIR}/out
)
 
target_link_options(securec_share PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)