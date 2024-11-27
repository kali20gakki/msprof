# libmspti.so
cmake_minimum_required(VERSION 3.14.1)
project(mspti_project)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_SOURCE_DIR}/../)

if(${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local")
   set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/build/prefix)
endif()

file(GLOB_RECURSE SRC_FILE
    ${CMAKE_SOURCE_DIR}/activity/*.cpp
    ${CMAKE_SOURCE_DIR}/callback/*.cpp
    ${CMAKE_SOURCE_DIR}/common/*.cpp
)

add_library(mspti_shared SHARED
    ${SRC_FILE}
)

set_target_properties(mspti_shared
    PROPERTIES
    OUTPUT_NAME mspti
)

target_include_directories(mspti_shared PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${TOP_DIR}/platform/securec/include
)

target_compile_options(mspti_shared PRIVATE
    -std=c++14
    -fPIC
    -fstack-protector-all
    -fno-strict-aliasing
    -fno-common
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wfloat-equal
    -Wextra
    -DMSPTI_LIB
    -D_FORTIFY_SOURCE=2
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-O2>
)

target_link_directories(mspti_shared PRIVATE
    ${SECUREC_LIB_DIR}
)

target_link_options(mspti_shared PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-s>
)

target_link_libraries(mspti_shared PRIVATE
    -Wl,--no-as-needed
    -Wl,--as-needed
    dl
    c_sec
    pthread
)

install(TARGETS mspti_shared OPTIONAL
    LIBRARY DESTINATION mspti
)

# **************************************************** mspti whl ********************************************
set(MSPTI_WHL_CSRC_DIR ${CMAKE_SOURCE_DIR}/whl/csrc)
add_subdirectory(${MSPTI_WHL_CSRC_DIR})
