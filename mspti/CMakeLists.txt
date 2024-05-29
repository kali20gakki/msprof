# libmspti.so
cmake_minimum_required(VERSION 3.14.1)
project(mspti_project)
set(CMAKE_SKIP_RPATH TRUE)

if(${CMAKE_INSTALL_PREFIX} STREQUAL "/usr/local")
   set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../build/mspti)
endif()

set(SECUREC_DIR
    ${CMAKE_SOURCE_DIR}/../platform/securec)

function(securec_compile)
    if(NOT EXISTS ${SECUREC_DIR})
        message("Please Download securec.")
    endif()
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
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}
    )
endfunction()

securec_compile()


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
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}
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
)

target_link_directories(mspti_shared PRIVATE
    ${CMAKE_INSTALL_PREFIX}
)

target_link_options(mspti_shared PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
)

target_link_libraries(mspti_shared PRIVATE
    -Wl,--no-as-needed
    -Wl,--as-needed
    dl
    c_sec
    pthread
)
