cmake_minimum_required(VERSION 3.14.1)
project(prof_common_project)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_SOURCE_DIR}/../)

if(${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/build/prefix)
endif()

file(GLOB_RECURSE SRC_FILE
        ${CMAKE_SOURCE_DIR}/cann/*.cpp
        ${CMAKE_SOURCE_DIR}/mstx/*.cpp
        ${TOP_DIR}/mspti/common/inject/plog_inject.cpp
        ${TOP_DIR}/mspti/common/function_loader.cpp
        ${TOP_DIR}/mspti/common/utils.cpp
)

add_library(prof_common_shared SHARED
        ${SRC_FILE}
)

set_target_properties(prof_common_shared
        PROPERTIES
        OUTPUT_NAME prof_common
)

target_include_directories(prof_common_shared PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${TOP_DIR}
        ${TOP_DIR}/mspti
        ${TOP_DIR}/platform/securec/include
)

message(STATUS, "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")

target_compile_options(prof_common_shared PRIVATE
        -std=c++14
        -fPIC
        -fstack-protector-all
        -fno-strict-aliasing
        -fno-common
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -Wfloat-equal
        -Wextra
        -Werror=uninitialized
        -Werror=return-type
        -Wall
        -D_FORTIFY_SOURCE=2
        $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-O2>
)

target_link_directories(prof_common_shared PRIVATE
        ${SECUREC_LIB_DIR}
)

target_link_options(prof_common_shared PRIVATE
        -Wl,-z,relro,-z,now,-z,noexecstack
        -Wl,-Bsymbolic
        -Wl,--exclude-libs=ALL
        $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-s>
)

target_link_libraries(prof_common_shared PRIVATE
        -Wl,--no-as-needed
        -Wl,--as-needed
)

install(TARGETS prof_common_shared OPTIONAL
        LIBRARY DESTINATION profcommon
)