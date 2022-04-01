###################################################################
set(logDaemonSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/log_recv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log_daemon.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log_recv_interface.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../slog/common/common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../slog/slog/src/log_monitor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/msg_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_to_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/start_single_process.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_common_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_sys_package.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/log_zip.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/cfg_file_parse.c
)

set(logDaemonHeaderDir
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/libkmc/include
    ${TOP_DIR}/libkmc/src/sdp
    ${TOP_DIR}/libkmc/src/common
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/config/user_config
    ${TOP_DIR}/abl/slog/shared
    ${TOP_DIR}/abl/slog/syslog/slog/common
    ${TOP_DIR}/abl/slog/syslog/slog/slog/src
    ${TOP_DIR}/abl/slog/syslog/hdclog/hdclog_host
    ${TOP_DIR}/abl/adump
    ${TOP_DIR}/abl/adump/external
    ${TOP_DIR}/abl/adump/hdc-common
)

###################################################################
if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend610")
    list(APPEND logDaemonHeaderDir
        ${IAM_HEADER_DIR}/include
        ${MDC_LINUX_SDK_DIR}/include)
endif ()
endif ()

add_executable(logDaemon
    ${logDaemonSrcFiles})

target_include_directories(logDaemon PRIVATE
    ${logDaemonHeaderDir})

target_compile_definitions(logDaemon PRIVATE
    $<$<STREQUAL:${PRODUCT},ascend310rc>:RECV_BY_DRV RC_MODE>
    $<$<STREQUAL:${PRODUCT},ascend320rc>:RECV_BY_DRV RC_MODE>
    $<$<STREQUAL:${PRODUCT},ascend320rcesl>:RECV_BY_DRV RC_MODE>
    $<$<STREQUAL:${PRODUCT},ascend610>:IAM SORTED_LOG RECV_BY_DRV HARDWARE_ZIP GETCLOCK_VIRTUAL RC_MODE LINUX_ETC_CONF>
    $<$<STREQUAL:${PRODUCT},ascend615>:SORTED_LOG RECV_BY_DRV HARDWARE_ZIP GETCLOCK_VIRTUAL RC_MODE LINUX_ETC_CONF>
    $<$<STREQUAL:${PRODUCT},ascend310>:RECV_BY_DRV _LOG_MONITOR_ _SLOG_DEVICE_ EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},ascend710>:RECV_BY_DRV _LOG_MONITOR_ EP_DEVICE_MODE _SLOG_DEVICE_>
    $<$<STREQUAL:${PRODUCT},helper710>:RECV_BY_DRV _LOG_MONITOR_ EP_DEVICE_MODE _SLOG_DEVICE_>
    $<$<STREQUAL:${PRODUCT},ascend910>:RECV_BY_DRV _LOG_MONITOR_ EP_DEVICE_MODE _SLOG_DEVICE_>
    $<$<STREQUAL:${PRODUCT},ascend920>:RECV_BY_DRV _LOG_MONITOR_ EP_DEVICE_MODE _SLOG_DEVICE_>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:RECV_BY_DRV _LOG_MONITOR_ EP_DEVICE_MODE _SLOG_DEVICE_>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:RECV_BY_DRV _LOG_MONITOR_ EP_DEVICE_MODE _SLOG_DEVICE_>
    OS_TYPE_DEF=0
    IDE_DAEMON_HOST
    _LOG_DAEMON_T_
)

target_compile_options(logDaemon PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -Werror
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(logDaemon PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_directories(logDaemon PRIVATE
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
)

if (PRODUCT)
target_link_libraries(logDaemon PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${PRODUCT},ascend910>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend910>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend310>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend310>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend710>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend710>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},helper710>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},helper710>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend610>:iam>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend610>:easy_comm>
    $<$<STREQUAL:${PRODUCT},ascend610>:wd>
    $<$<STREQUAL:${PRODUCT},ascend610>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend615>:iam>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend615>:easy_comm>
    $<$<STREQUAL:${PRODUCT},ascend615>:wd>
    $<$<STREQUAL:${PRODUCT},ascend615>:ascend_hal>
    c_sec
    slog
    mmpa
    bbox_proc
)
endif ()

set_target_properties(logDaemon
    PROPERTIES
    OUTPUT_NAME log-daemon
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

###################################################################
if (PRODUCT)
if ((${PRODUCT} STREQUAL "ascend310") OR (${PRODUCT} STREQUAL "ascend710") OR (${PRODUCT} STREQUAL "ascend910") OR (${PRODUCT} STREQUAL "ascend920") OR (${PRODUCT} STREQUAL "ascend920esl") OR (${PRODUCT} STREQUAL "ascend920emu") OR (${PRODUCT} STREQUAL "helper710"))
add_custom_target(logdaemonDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/log_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend610" OR (${PRODUCT} STREQUAL "ascend615"))
add_custom_target(logdaemonDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../slog/slog/etc/1951_mdc/log-daemon.yaml ${CMAKE_CURRENT_BINARY_DIR}
)
endif ()
endif ()

###################################################################
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR  lib/cmake)

include(CMakePackageConfigHelpers)

if (PRODUCT)
if ((${PRODUCT} STREQUAL "ascend310") OR (${PRODUCT} STREQUAL "ascend910") OR (${PRODUCT} STREQUAL "ascend710") OR (${PRODUCT} STREQUAL "ascend920") OR (${PRODUCT} STREQUAL "ascend920esl") OR (${PRODUCT} STREQUAL "ascend920emu") OR (${PRODUCT} STREQUAL "helper710"))
install(TARGETS logDaemon OPTIONAL
    EXPORT hdclog_device_targets
    LIBRARY OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/log_daemon_monitor.sh OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
elseif ((${PRODUCT} STREQUAL "ascend310rc") OR (${PRODUCT} STREQUAL "ascend320rc") OR (${PRODUCT} STREQUAL "ascend320rcesl"))
install(TARGETS logDaemon OPTIONAL
    LIBRARY OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
elseif ((${PRODUCT} STREQUAL "ascend610") OR (${PRODUCT} STREQUAL "ascend615"))
install(TARGETS logDaemon OPTIONAL
    LIBRARY OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
    ARCHIVE OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/log-daemon.yaml OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif ()
endif ()
