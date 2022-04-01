if (HAVE_OPENSSL)
    return()
endif()

include(ExternalProject)
include(GNUInstallDirs)

if ((${CMAKE_INSTALL_PREFIX} STREQUAL /usr/local) OR
    (${CMAKE_INSTALL_PREFIX} STREQUAL "C:/Program Files (x86)/ascend"))
    set(CMAKE_INSTALL_PREFIX ${AIR_CODE_DIR}/output CACHE STRING "path for install()" FORCE)
    message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}.")
endif ()

set(OPENSSL_PKG_DIR ${CMAKE_INSTALL_PREFIX}/openssl)

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL x86_64)
    set(OPENSSL_HOST_LOCAL_ARCH linux-x86_64)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL aarch64)
    set(OPENSSL_HOST_LOCAL_ARCH linux-aarch64)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL arm)
    set(OPENSSL_HOST_LOCAL_ARCH linux-armv4)
endif ()

set(SAFE_SP_OPTIONS -fstack-protector-strong)

set(COMPILE_OPTIONE -D_FORFIFY_SOURCE=2 -O2 -Wl,-z,relro,-z,now,-z,noexecstack$<$<CONFIG:Release>:,--build-id=none>)

if (AIR_PB_PKG)
    set(REQ_URL "${AIR_PB_PKG}/libs/openssl/OpenSSL_1_1_1k.tar.gz")
    set(MD5 "")
elseif (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/openssl/repository/archive/OpenSSL_1_1_1k.tar.gz")
    set(MD5 "")
else ()
    set(REQ_URL "https://github.com/openssl/openssl/archive/refs/tags/OpenSSL_1_1_1k.tar.gz")
    set(MD5 "")
endif ()

ExternalProject_Add(openssl_build
    URL ${REQ_URL}
    TLS_VERIFY OFF
    CONFIGURE_COMMAND <SOURCE_DIR>/Configure ${OPENSSL_HOST_LOCAL_ARCH} no-asm enable-shared threads enable-ssl3-method ${SAFE_SP_OPTIONS} ${COMPILE_OPTIONE} --prefix=${OPENSSL_PKG_DIR}
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND $(MAKE) install_sw
    EXCLUDE_FROM_ALL TRUE
)

file(MAKE_DIRECTORY ${OPENSSL_PKG_DIR}/include)

# ----- crypto_static ------
set(CRYPTO_STATIC ${OPENSSL_PKG_DIR}/lib/libcrypto.a)
add_library(crypto_static STATIC IMPORTED)
set_target_properties(crypto_static PROPERTIES
        IMPORTED_LOCATION ${CRYPTO_STATIC}
)
target_include_directories(crypto_static INTERFACE ${OPENSSL_PKG_DIR}/include)
add_dependencies(crypto_static openssl_build)

install(FILES ${CRYPTO_STATIC}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        OPTIONAL
)

# ----- ssl_static ------
set(SSL_STATIC ${OPENSSL_PKG_DIR}/lib/libssl.a)
add_library(ssl_static STATIC IMPORTED)
set_target_properties(ssl_static PROPERTIES
        IMPORTED_LOCATION ${SSL_STATIC}
)
target_include_directories(ssl_static INTERFACE ${OPENSSL_PKG_DIR}/include)
add_dependencies(ssl_static openssl_build)

install(FILES ${SSL_STATIC}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        OPTIONAL
)

set(HAVE_OPENSSL TRUE)
