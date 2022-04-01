if (HAVE_GRPC)
    return()
endif()

include(ExternalProject)

if ((${CMAKE_INSTALL_PREFIX} STREQUAL /usr/local) OR
    (${CMAKE_INSTALL_PREFIX} STREQUAL "C:/Program Files (x86)/ascend"))
    set(CMAKE_INSTALL_PREFIX ${AIR_CODE_DIR}/output CACHE STRING "path for install()" FORCE)
    message (STATUS "No install prefix selected, default to${CMAKE_INSTALL_PREFIX}.")
endif ()

if (AIR_PB_PKG)
    set(REQ_URL "${AIR_PB_PKG}/libs/grpc/grpc-1.36.1.tar.gz")
    set(MD5 "")
elseif (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/grpc/repository/archive/v1.36.1.tar.gz")
    set(MD5 "")
else ()
    set(REQ_URL "https://github.com/grpc/grpc/archive/refs/tags/v1.36.1.tar.gz")
    set(MD5 "")
endif ()

set(GRPC_CXX_FLAGS "-Wl,-z,relro,-z,now,-z,noexecstack -D_FORTIFY_SOURCE=2 -O2 -fstack-protector-all -s -D_GLIBCXX_USE_CXX11_ABI=0")

set(GRPC_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/grpc)

ExternalProject_Add(grpc_build
                    URL ${REQ_URL}
                    PATCH_COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/zlib_build-prefix/src/zlib_build <SOURCE_DIR>/third_party/zlib
		    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                        # zlib
                        -DgRPC_ZLIB_PROVIDER=module
                        # cares
                        -DgRPC_CARES_PROVIDER=module
                        -DCARES_ROOT_DIR=${CMAKE_BINARY_DIR}/cares_build-prefix/src/cares_build
                        # re2
                        -DgRPC_RE2_PROVIDER=module
                        -DRE2_ROOT_DIR=${CMAKE_BINARY_DIR}/re2_build-prefix/src/re2_build
                        # absl
                        -DgRPC_ABSL_PROVIDER=module
                        -DABSL_ROOT_DIR=${CMAKE_BINARY_DIR}/abseil_build-prefix/src/abseil_build
                        # protobuf
                        -DgRPC_PROTOBUF_PROVIDER=module
                        -DPROTOBUF_ROOT_DIR=${CMAKE_BINARY_DIR}/protobuf_build-prefix/src/protobuf_build
                        # ssl
                        -DgRPC_SSL_PROVIDER=package
                        -DOPENSSL_ROOT_DIR=${CMAKE_INSTALL_PREFIX}/openssl
                        -DOPENSSL_USE_STATIC_LIBS=TRUE
                        # grpc option
                        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                        -DCMAKE_CXX_FLAGS=${GRPC_CXX_FLAGS}
                        -DCMAKE_BUILD_TYPE=Release
                        -DgRPC_BUILD_TESTS=OFF
                        -DCMAKE_INSTALL_LIBDIR=lib
                        -DgRPC_BUILD_CSHARP_EXT=OFF
                        -DgRPC_BUILD_CODEGEN=OFF
                        -DgRPC_BUILD_GRPC_CPP_PLUGIN=OFF
                        -DCMAKE_INSTALL_PREFIX=${GRPC_INSTALL_DIR}
                        <SOURCE_DIR>
                    BUILD_COMMAND $(MAKE)
                    INSTALL_COMMAND $(MAKE) install
                    EXCLUDE_FROM_ALL TRUE
)
add_dependencies(grpc_build openssl_build re2_build zlib_build cares_build abseil_build protobuf_build)
file(MAKE_DIRECTORY ${GRPC_INSTALL_DIR}/include)

add_library(grpc STATIC IMPORTED)
set_target_properties(grpc PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libgrpc.a
	INTERFACE_LINK_LIBRARIES "ssl_static;crypto_static;gRPC::zlibstatic;gRPC::cares;gRPC::address_sorting;gRPC::re2;gRPC::upb;dl;rt;m;pthread;gRPC::gpr;gRPC::address_sorting;gRPC::upb;gRPC::absl_statusor;gRPC::absl_hash;gRPC::absl_bad_variant_access;gRPC::absl_city;gRPC::absl_raw_hash_set;gRPC::absl_hashtablez_sampler;gRPC::absl_exponential_biased;gRPC::absl_status;gRPC::absl_bad_optional_access;gRPC::absl_synchronization;gRPC::absl_stacktrace;gRPC::absl_symbolize;gRPC::absl_debugging_internal;gRPC::absl_demangle_internal;gRPC::absl_graphcycles_internal;gRPC::absl_time;gRPC::absl_civil_time;gRPC::absl_time_zone;gRPC::absl_malloc_internal;gRPC::absl_str_format_internal;gRPC::absl_strings;gRPC::absl_strings_internal;gRPC::absl_int128;gRPC::absl_throw_delegate;gRPC::absl_base;gRPC::absl_raw_logging_internal;gRPC::absl_log_severity;gRPC::absl_spinlock_wait"
	)
target_include_directories(grpc INTERFACE ${GRPC_INSTALL_DIR}/include)
add_dependencies(grpc grpc_build)

add_library(grpc++ STATIC IMPORTED)
set_target_properties(grpc++ PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libgrpc++.a
        INTERFACE_LINK_LIBRARIES "gRPC::protobuf;dl;rt;m;pthread;grpc;gRPC::gpr;gRPC::address_sorting;gRPC::upb"
        )
target_include_directories(grpc++ INTERFACE ${GRPC_INSTALL_DIR}/include)
add_dependencies(grpc++ grpc_build)

add_library(gRPC::protobuf STATIC IMPORTED)
set_target_properties(gRPC::protobuf PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libprotobuf.a
        )
add_dependencies(gRPC::protobuf grpc_build)

add_library(gRPC::re2 STATIC IMPORTED)
set_target_properties(gRPC::re2 PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libre2.a
        )
add_dependencies(gRPC::re2 grpc_build)

add_library(gRPC::gpr STATIC IMPORTED)
set_target_properties(gRPC::gpr PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libgpr.a
	INTERFACE_LINK_LIBRARIES "dl;rt;m;pthread;gRPC::absl_statusor;gRPC::absl_hash;gRPC::absl_bad_variant_access;gRPC::absl_city;gRPC::absl_raw_hash_set;gRPC::absl_hashtablez_sampler;gRPC::absl_exponential_biased;gRPC::absl_status;gRPC::absl_cord;gRPC::absl_bad_optional_access;gRPC::absl_synchronization;gRPC::absl_stacktrace;gRPC::absl_symbolize;gRPC::absl_debugging_internal;gRPC::absl_demangle_internal;gRPC::absl_graphcycles_internal;gRPC::absl_time;gRPC::absl_civil_time;gRPC::absl_time_zone;gRPC::absl_malloc_internal;gRPC::absl_str_format_internal;gRPC::absl_strings;gRPC::absl_strings_internal;gRPC::absl_int128;gRPC::absl_throw_delegate;gRPC::absl_base;gRPC::absl_raw_logging_internal;gRPC::absl_log_severity;gRPC::absl_spinlock_wait;"
        )
add_dependencies(gRPC::gpr grpc_build)

add_library(gRPC::upb STATIC IMPORTED)
set_target_properties(gRPC::upb PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libupb.a
        INTERFACE_LINK_LIBRARIES "dl;rt;m;pthread"
        )
add_dependencies(gRPC::upb grpc_build)

add_library(gRPC::cares STATIC IMPORTED)
set_target_properties(gRPC::cares PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libcares.a
        )
add_dependencies(gRPC::cares grpc_build)

add_library(gRPC::address_sorting STATIC IMPORTED)
set_target_properties(gRPC::address_sorting PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libaddress_sorting.a
        INTERFACE_LINK_LIBRARIES "dl;rt;m;pthread"
        )
add_dependencies(gRPC::address_sorting grpc_build)

add_library(gRPC::zlibstatic STATIC IMPORTED)
set_target_properties(gRPC::zlibstatic PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libz.a
        )
add_dependencies(gRPC::zlibstatic grpc_build)

add_library(gRPC::absl_statusor STATIC IMPORTED)
set_target_properties(gRPC::absl_statusor PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_statusor.a
        )
add_dependencies(gRPC::absl_statusor grpc_build)

add_library(gRPC::absl_hash STATIC IMPORTED)
set_target_properties(gRPC::absl_hash PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_hash.a
        )
add_dependencies(gRPC::absl_hash grpc_build)

add_library(gRPC::absl_bad_variant_access STATIC IMPORTED)
set_target_properties(gRPC::absl_bad_variant_access PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_bad_variant_access.a
        )
add_dependencies(gRPC::absl_bad_variant_access grpc_build)

add_library(gRPC::absl_city STATIC IMPORTED)
set_target_properties(gRPC::absl_city PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_city.a
        )
add_dependencies(gRPC::absl_city grpc_build)

add_library(gRPC::absl_raw_hash_set STATIC IMPORTED)
set_target_properties(gRPC::absl_raw_hash_set PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_raw_hash_set.a
        )
add_dependencies(gRPC::absl_raw_hash_set grpc_build)

add_library(gRPC::absl_hashtablez_sampler STATIC IMPORTED)
set_target_properties(gRPC::absl_hashtablez_sampler PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_hashtablez_sampler.a
        )
add_dependencies(gRPC::absl_hashtablez_sampler grpc_build)

add_library(gRPC::absl_exponential_biased STATIC IMPORTED)
set_target_properties(gRPC::absl_exponential_biased PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_exponential_biased.a
        )
add_dependencies(gRPC::absl_exponential_biased grpc_build)

add_library(gRPC::absl_status STATIC IMPORTED)
set_target_properties(gRPC::absl_status PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_status.a
        )
add_dependencies(gRPC::absl_status grpc_build)

add_library(gRPC::absl_cord STATIC IMPORTED)
set_target_properties(gRPC::absl_cord PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_cord.a
        )
add_dependencies(gRPC::absl_cord grpc_build)

add_library(gRPC::absl_bad_optional_access STATIC IMPORTED)
set_target_properties(gRPC::absl_bad_optional_access PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_bad_optional_access.a
        )
add_dependencies(gRPC::absl_bad_optional_access grpc_build)

add_library(gRPC::absl_synchronization STATIC IMPORTED)
set_target_properties(gRPC::absl_synchronization PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_synchronization.a
        )
add_dependencies(gRPC::absl_synchronization grpc_build)

add_library(gRPC::absl_stacktrace STATIC IMPORTED)
set_target_properties(gRPC::absl_stacktrace PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_stacktrace.a
        )
add_dependencies(gRPC::absl_stacktrace grpc_build)

add_library(gRPC::absl_symbolize STATIC IMPORTED)
set_target_properties(gRPC::absl_symbolize PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_symbolize.a
        )
add_dependencies(gRPC::absl_symbolize grpc_build)

add_library(gRPC::absl_debugging_internal STATIC IMPORTED)
set_target_properties(gRPC::absl_debugging_internal PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_debugging_internal.a
        )
add_dependencies(gRPC::absl_debugging_internal grpc_build)

add_library(gRPC::absl_demangle_internal STATIC IMPORTED)
set_target_properties(gRPC::absl_demangle_internal PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_demangle_internal.a
        )
add_dependencies(gRPC::absl_demangle_internal grpc_build)

add_library(gRPC::absl_graphcycles_internal STATIC IMPORTED)
set_target_properties(gRPC::absl_graphcycles_internal PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_graphcycles_internal.a
        )
add_dependencies(gRPC::absl_graphcycles_internal grpc_build)

add_library(gRPC::absl_time STATIC IMPORTED)
set_target_properties(gRPC::absl_time PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_time.a
        )
add_dependencies(gRPC::absl_time grpc_build)

add_library(gRPC::absl_civil_time STATIC IMPORTED)
set_target_properties(gRPC::absl_civil_time PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_civil_time.a
        )
add_dependencies(gRPC::absl_civil_time grpc_build)

add_library(gRPC::absl_time_zone STATIC IMPORTED)
set_target_properties(gRPC::absl_time_zone PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_time_zone.a
        )
add_dependencies(gRPC::absl_time_zone grpc_build)

add_library(gRPC::absl_malloc_internal STATIC IMPORTED)
set_target_properties(gRPC::absl_malloc_internal PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_malloc_internal.a
        )
add_dependencies(gRPC::absl_malloc_internal grpc_build)

add_library(gRPC::absl_str_format_internal STATIC IMPORTED)
set_target_properties(gRPC::absl_str_format_internal PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_str_format_internal.a
        )
add_dependencies(gRPC::absl_str_format_internal grpc_build)

add_library(gRPC::absl_strings STATIC IMPORTED)
set_target_properties(gRPC::absl_strings PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_strings.a
        )
add_dependencies(gRPC::absl_strings grpc_build)

add_library(gRPC::absl_strings_internal STATIC IMPORTED)
set_target_properties(gRPC::absl_strings_internal PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_strings_internal.a
        )
add_dependencies(gRPC::absl_strings_internal grpc_build)

add_library(gRPC::absl_int128 STATIC IMPORTED)
set_target_properties(gRPC::absl_int128 PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_int128.a
        )
add_dependencies(gRPC::absl_int128 grpc_build)

add_library(gRPC::absl_throw_delegate STATIC IMPORTED)
set_target_properties(gRPC::absl_throw_delegate PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_throw_delegate.a
        )
add_dependencies(gRPC::absl_throw_delegate grpc_build)

add_library(gRPC::absl_base STATIC IMPORTED)
set_target_properties(gRPC::absl_base PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_base.a
        )
add_dependencies(gRPC::absl_base grpc_build)

add_library(gRPC::absl_raw_logging_internal STATIC IMPORTED)
set_target_properties(gRPC::absl_raw_logging_internal PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_raw_logging_internal.a
        )
add_dependencies(gRPC::absl_raw_logging_internal grpc_build)

add_library(gRPC::absl_log_severity STATIC IMPORTED)
set_target_properties(gRPC::absl_log_severity PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_log_severity.a
        )
add_dependencies(gRPC::absl_log_severity grpc_build)

add_library(gRPC::absl_spinlock_wait STATIC IMPORTED)
set_target_properties(gRPC::absl_spinlock_wait PROPERTIES
        IMPORTED_LOCATION ${GRPC_INSTALL_DIR}/lib/libabsl_spinlock_wait.a
        )
add_dependencies(gRPC::absl_spinlock_wait grpc_build)

set(HAVE_GRPC TRUE)
