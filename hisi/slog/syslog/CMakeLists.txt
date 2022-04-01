cmake_minimum_required(VERSION 3.12)
project(toolchain_syslog)

add_subdirectory(slog)
add_subdirectory(hdclog)
if (${PRODUCT_SIDE} STREQUAL "host")
add_subdirectory(msnpureport)
endif ()
