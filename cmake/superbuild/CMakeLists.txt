cmake_minimum_required(VERSION 3.14.1)
project(msprof_superbuild)

include(ExternalProject)

get_filename_component(TOP_DIR "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if (CMAKE_INSTALL_PREFIX STREQUAL /usr/local)
    set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/tmp CACHE STRING "path for install()" FORCE)
    message(STATUS "No install prefix selected, default to ${CMAKE_INSTALL_PREFIX}.")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/external/ascend_protobuf_shared.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/external/ascend_protobuf_static.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/external/protoc.cmake)

ExternalProject_Add(msprof
    SOURCE_DIR ${TOP_DIR}
    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
                <SOURCE_DIR>
    BUILD_COMMAND $(MAKE)
    DEPENDS protoc_build ascend_protobuf_shared_build ascend_protobuf_static_build
)
