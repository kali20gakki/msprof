cmake_minimum_required(VERSION 3.14.1)
project(msprof_superbuild)

include(ExternalProject)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if (CMAKE_INSTALL_PREFIX STREQUAL /usr/local)
    set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/build/prefix CACHE STRING "path for install()" FORCE)
    message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}.")
endif()

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

ExternalProject_Add(analysis
    SOURCE_DIR ${TOP_DIR}/analysis/csrc
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DSECUREC_LIB_DIR=${CMAKE_INSTALL_PREFIX}/securec_shared
                <SOURCE_DIR>
    BUILD_COMMAND $(MAKE)
    DEPENDS c_sec
)
