###################################################################
set(libhdclogDeviceSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/hdclog_device_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_common_util.c
)

set(libhdclogDeviceDir
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/abl/slog/shared
    ${TOP_DIR}/abl/slog/syslog/hdclog/hdclog_device
    ${TOP_DIR}/abl/adump/hdc-common
)

###################################################################
add_library(hdclog_device STATIC
    ${libhdclogDeviceSrcFiles})

target_include_directories(hdclog_device PRIVATE
    ${libhdclogDeviceDir})

target_compile_definitions(hdclog_device PRIVATE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
)

target_compile_options(hdclog_device PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -Werror
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(hdclog_device PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_libraries(hdclog_device PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    c_sec
    mmpa
)

set_target_properties(hdclog_device
    PROPERTIES
    OUTPUT_NAME hdclog_device
)

###################################################################
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR  lib/cmake)

include(CMakePackageConfigHelpers)

if (PRODUCT)
if ((${PRODUCT} STREQUAL "ascend310") OR (${PRODUCT} STREQUAL "ascend910") OR (${PRODUCT} STREQUAL "ascend710") OR (${PRODUCT} STREQUAL "helper710"))
install(TARGETS hdclog_device OPTIONAL
    EXPORT hdclog_device_targets
    LIBRARY OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif ()
endif ()
