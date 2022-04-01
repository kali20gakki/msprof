cmake_minimum_required(VERSION 3.14.1)
project(slog_aicpu_stub)
get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" ABSOLUTE)
set(LOCAL_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/slog_stub.cpp
)

add_library(slog_aicpu_stub SHARED
    ${LOCAL_SRC_FILES}
)

target_include_directories(slog_aicpu_stub PRIVATE
    ${TOP_DIR}/third_party/inc
)

target_compile_options(slog_aicpu_stub PRIVATE
    -std=c++11
    -fPIC
)

#[[add_library(alog SHARED
    ${LOCAL_SRC_FILES}
)

target_include_directories(alog PRIVATE
    ${TOP_DIR}/third_party/inc
)

target_compile_options(alog PRIVATE
    -std=c++11
    -fPIC
)]]
