cmake_minimum_required(VERSION 3.14)
project(msprof_llt_superbuild)

include(ExternalProject)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if (CMAKE_INSTALL_PREFIX STREQUAL /usr/local)
    set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/test/output CACHE STRING "path for install()" FORCE)
    message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}.")
endif()

include(${TOP_DIR}/cmake/superbuild/external/ascend_protobuf_shared.cmake)
include(${TOP_DIR}/cmake/superbuild/external/ascend_protobuf_static.cmake)
include(${TOP_DIR}/cmake/superbuild/external/protoc.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/opensource/googletest.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/opensource/mockcpp.cmake)

ExternalProject_Add(msprof_llt
    SOURCE_DIR ${TOP_DIR}/test/msprof_cpp
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                -DPACKAGE=${PACKAGE}
                -DFULL_COVERAGE=true
                -DCOVERAGE_RC_CONFIG=true
                <SOURCE_DIR>
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND "" # llt install
    BUILD_ALWAYS TRUE
    DEPENDS protoc_build ascend_protobuf_shared_build ascend_protobuf_static_build gtest_build mockcpp_build
)
