cmake_minimum_required(VERSION 3.14.1)
project(msprof_superbuild)

include(ExternalProject)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if (CMAKE_INSTALL_PREFIX STREQUAL /usr/local)
    set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/build/prefix CACHE STRING "path for install()" FORCE)
    message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}.")
endif()

# include(${CMAKE_CURRENT_LIST_DIR}/external/ascend_protobuf_shared.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/external/ascend_protobuf_static.cmake)
# include(${CMAKE_CURRENT_LIST_DIR}/external/protoc.cmake)

########################## securec ###################################
set(SECUREC_DIR ${TOP_DIR}/platform/securec)
file(GLOB_RECURSE SECUREC_SRC ${SECUREC_DIR}/src/*.c)

add_library(c_sec SHARED
    ${SECUREC_SRC}
)

target_include_directories(c_sec PRIVATE
    ${SECUREC_DIR}/include
)

target_compile_options(c_sec PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

target_link_options(c_sec PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

set_target_properties(c_sec
    PROPERTIES
    OUTPUT_NAME c_sec
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/securec_shared
)

########################## error_manager ###################################
add_library(error_manager SHARED
    ${TOP_DIR}/collector/dvvp/depend/stub/error_manager/error_manager_stub.cpp
)

target_include_directories(error_manager PRIVATE
    ${TOP_DIR}/collector/dvvp/depend/inc/metadef
)

target_compile_options(error_manager PRIVATE
    -std=c++11
    -Werror
    -fno-common
    -Wextra
    -Wfloat-equal
    -D_GLIBCXX_USE_CXX11_ABI=0
)

set_target_properties(error_manager
    PROPERTIES
    OUTPUT_NAME error_manager
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/error_manager
)

if(OBJECT STREQUAL all OR OBJECT STREQUAL collector)
ExternalProject_Add(msprof
    SOURCE_DIR ${TOP_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_EXPORT_COMPILE_COMMANDS=1
                -DSECUREC_LIB_DIR=${CMAKE_INSTALL_PREFIX}/securec_shared
                -DERROR_MANAGER_LIB_DIR=${CMAKE_INSTALL_PREFIX}/error_manager
                -DHITEST=${HITEST}
                <SOURCE_DIR>
    BUILD_COMMAND $(MAKE)
)
endif()

if(OBJECT STREQUAL all OR OBJECT STREQUAL analysis)
ExternalProject_Add(analysis
    SOURCE_DIR ${TOP_DIR}/analysis/csrc
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DSECUREC_LIB_DIR=${CMAKE_INSTALL_PREFIX}/securec_shared
                <SOURCE_DIR>
    BUILD_COMMAND $(MAKE)
    DEPENDS c_sec
)
endif()

if(OBJECT STREQUAL all OR OBJECT STREQUAL mspti)
ExternalProject_Add(mspti
    SOURCE_DIR ${TOP_DIR}/mspti
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DSECUREC_LIB_DIR=${CMAKE_INSTALL_PREFIX}/securec_shared
                <SOURCE_DIR>
    BUILD_COMMAND $(MAKE)
    DEPENDS c_sec
)
endif()
