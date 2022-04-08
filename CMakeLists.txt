# msprofbin libmsprofiler.so stub/libmsprofiler.so
cmake_minimum_required(VERSION 3.14.1)

project(msprof)
set(TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(INSTALL_DIR ${TOP_DIR}/install)

add_subdirectory(opensource)
add_subdirectory(collector)


