cmake_minimum_required(VERSION 3.14.1)
project(mspti_project)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_SOURCE_DIR})

if(${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local")
   set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/prefix)
endif()

file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX})
file(MAKE_DIRECTORY ${SECUREC_LIB_DIR})

# **************************************************** securec ********************************************
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
    LIBRARY_OUTPUT_DIRECTORY ${SECUREC_LIB_DIR}
)

# **************************************************** mspti ********************************************

file(GLOB_RECURSE SRC_FILE
    ${CMAKE_SOURCE_DIR}/csrc/activity/*.cpp
    ${CMAKE_SOURCE_DIR}/csrc/callback/*.cpp
    ${CMAKE_SOURCE_DIR}/csrc/common/*.cpp
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
    ${SECUREC_DIR}/include
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
    -Werror=uninitialized
    -Werror=return-type
    -Wall
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
set(MSPTI_WHL_CSRC_DIR ${CMAKE_SOURCE_DIR}/mspti/csrc)
add_subdirectory(${MSPTI_WHL_CSRC_DIR})
