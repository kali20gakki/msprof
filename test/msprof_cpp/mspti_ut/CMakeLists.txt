cmake_minimum_required(VERSION 3.14.1)
project(msprof_llt)
add_definitions(-DMSPORF_LLT)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

set(CMAKE_MODULE_PATH
    ${TOP_DIR}/cmake/modules
    ${TOP_DIR}/test/cmake/modules
)

include(${TOP_DIR}/test/cmake/depend.cmake)

find_package(gtest)

set(CMAKE_CXX_FLAGS " -Wno-unused")

# common include
include_directories(
    ${TOP_DIR}/test/msprof_cpp/mspti_ut/stub
    ${TOP_DIR}/mspti
    ${TOP_DIR}/mspti/external
    ${TOP_DIR}/platform/securec/include
    ${GTEST_INCLUDE_DIR}
)

# common compile option
add_compile_options(
    -g
    -pipe
    -Wall
    -std=c++14
    -fno-access-control
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
    -fprofile-arcs
    -ftest-coverage
)

# common link options
add_link_options(
    -lgcov
    --coverage
)

# common link library
link_libraries(
    ${GTEST_STATIC_LIBRARY}
    c_sec_static
    pthread
    dl
)

add_subdirectory(callback)
add_subdirectory(activity)
