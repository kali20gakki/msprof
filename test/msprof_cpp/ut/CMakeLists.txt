cmake_minimum_required(VERSION 3.14.1)
project(msprof_llt)
add_definitions(-DMSPORF_LLT)
set(CMAKE_SKIP_RPATH TRUE)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
set(MSPROF_SOURCE_DIR ${TOP_DIR}/collector)
set(CMAKE_MODULE_PATH
    ${TOP_DIR}/cmake/modules
    ${TOP_DIR}/test/cmake/modules
)

find_package(gtest)
find_package(mockcpp)

include(${TOP_DIR}/test/cmake/depend.cmake)

set(CMAKE_CXX_FLAGS " -Wno-unused")

add_subdirectory(transport)
add_subdirectory(msprof)
add_subdirectory(app)
add_subdirectory(common)
add_subdirectory(driver)
add_subdirectory(job_wrapper)
add_subdirectory(message)
add_subdirectory(streamio)
add_subdirectory(msprofbin)
add_subdirectory(stub_utest)
add_subdirectory(plugin)
add_subdirectory(params_adapter)
