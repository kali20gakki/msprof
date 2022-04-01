if (${PRODUCT_SIDE} STREQUAL "device")
###################################################################
set(slogdSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/syslogd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syslogd_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/operate_loglevel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/slogd_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log_monitor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log_dump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app_log_watch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_session_manage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/msg_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/share_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/cfg_file_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_zip.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_common_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_sys_package.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/start_single_process.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_to_file.c)

set(slogdHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/kernel/vendor/nve/inlcude/linux/mtd
    ${TOP_DIR}/inc/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${TOP_DIR}/abl/adump/external
    )

set(iamSlogdSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/syslogd_iam.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syslogd_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/operate_loglevel.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/msg_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/share_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/slogd_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log_monitor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/log_dump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/app_log_watch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_zip.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/start_single_process.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/cfg_file_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_common_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_sys_package.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_to_file.c
)

set(iamSlogdIncFiles
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../slog
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared
    ${TOP_DIR}/kernel/vendor/nve/inlcude/linux/mtd
    ${MDC_LINUX_SDK_DIR}/include
    ${IAM_HEADER_DIR}/include
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/abl/adump/external)


if (PRODUCT)
if ((${PRODUCT} STREQUAL "ascend310") OR (${PRODUCT} STREQUAL "ascend910") OR (${PRODUCT} STREQUAL "ascend710") OR (${PRODUCT} STREQUAL "ascend920") OR (${PRODUCT} STREQUAL "ascend920esl") OR (${PRODUCT} STREQUAL "ascend920emu") OR (${PRODUCT} STREQUAL "helper710"))
    list(APPEND slogdSrcFiles ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/external/log_drv.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/external/api/driver_api.c
    )
    list(APPEND slogdHeaderDir ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/external)
elseif (${PRODUCT} STREQUAL "ascend610")
    set(slogdSrcFiles ${iamSlogdSrcFiles})
    set(slogdHeaderDir ${iamSlogdIncFiles})
endif ()
endif ()

add_executable(slogd
    ${slogdSrcFiles})

target_include_directories(slogd PRIVATE
    ${slogdHeaderDir})

target_compile_definitions(slogd PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${PRODUCT},ascend310>:_LOG_MONITOR_ _SLOG_DEVICE_ STACKCORE LOG_COREDUMP APP_LOG_REPORT EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},ascend310rc>:_SLOG_DEVICE_ STACKCORE APP_LOG_WATCH RC_MODE>
    $<$<STREQUAL:${PRODUCT},ascend320rc>:_SLOG_DEVICE_ STACKCORE APP_LOG_WATCH RC_MODE>
    $<$<STREQUAL:${PRODUCT},ascend320rcesl>:_SLOG_DEVICE_ STACKCORE APP_LOG_WATCH RC_MODE>
    $<$<STREQUAL:${PRODUCT},ascend910>:_LOG_MONITOR_ _SLOG_DEVICE_ STACKCORE LOG_COREDUMP APP_LOG_REPORT EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},ascend920>:_LOG_MONITOR_ _SLOG_DEVICE_ STACKCORE LOG_COREDUMP APP_LOG_REPORT EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:_LOG_MONITOR_ _SLOG_DEVICE_ STACKCORE LOG_COREDUMP APP_LOG_REPORT EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:_LOG_MONITOR_ _SLOG_DEVICE_ STACKCORE LOG_COREDUMP APP_LOG_REPORT EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},npuf10>:LHISI_ APP_LOG_WATCH RC_HISI_MODE>
    $<$<STREQUAL:${PRODUCT},ascend610>:_SLOG_DEVICE_ HARDWARE_ZIP GETCLOCK_VIRTUAL SORTED_LOG APP_LOG_WATCH RC_MODE IAM LINUX_ETC_CONF>
    $<$<STREQUAL:${PRODUCT},ascend710>:_LOG_MONITOR_ _SLOG_DEVICE_ LOG_COREDUMP APP_LOG_REPORT EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},helper710>:_LOG_MONITOR_ _SLOG_DEVICE_ LOG_COREDUMP APP_LOG_REPORT EP_DEVICE_MODE>
    $<$<STREQUAL:${PRODUCT},ascend615>:_SLOG_DEVICE_ HARDWARE_ZIP GETCLOCK_VIRTUAL SORTED_LOG APP_LOG_WATCH RC_MODE LINUX_ETC_CONF>
)

target_compile_options(slogd PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -O2
    -Werror
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(slogd PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_directories(slogd PRIVATE
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
)

target_link_libraries(slogd PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<$<STREQUAL:${PRODUCT},ascend910>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend910>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend910>:stackcore>
    $<$<STREQUAL:${PRODUCT},ascend910>:log_server>
    $<$<STREQUAL:${PRODUCT},ascend920>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920>:stackcore>
    $<$<STREQUAL:${PRODUCT},ascend920>:log_server>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:stackcore>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:log_server>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:stackcore>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:log_server>
    $<$<STREQUAL:${PRODUCT},ascend710>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend710>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend710>:log_server>
    $<$<STREQUAL:${PRODUCT},helper710>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},helper710>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},helper710>:log_server>
    $<$<STREQUAL:${PRODUCT},ascend610>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend610>:wd>
    $<$<STREQUAL:${PRODUCT},ascend610>:iam>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend610>:easy_comm>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend615>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend615>:wd>
    $<$<STREQUAL:${PRODUCT},ascend615>:iam>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend615>:easy_comm>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend310>:stackcore>
    $<$<STREQUAL:${PRODUCT},ascend310>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend310>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend310>:log_server>
    $<$<STREQUAL:${PRODUCT},ascend310rc>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend310rc>:stackcore>
    $<$<STREQUAL:${PRODUCT},ascend320rc>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend320rc>:stackcore>
    $<$<STREQUAL:${PRODUCT},ascend320rcesl>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend320rcesl>:stackcore>
    c_sec
)

set_target_properties(slogd
    PROPERTIES
    OUTPUT_NAME slogd
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

###################################################################
if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend310")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/mini/slog.conf ${CMAKE_CURRENT_BINARY_DIR}
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/slogd_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend310rc" OR ${PRODUCT} STREQUAL "ascend320rc" OR ${PRODUCT} STREQUAL "ascend320rcesl")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/mini_rc/slog.conf ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend910" OR ${PRODUCT} STREQUAL "ascend920" OR ${PRODUCT} STREQUAL "ascend920esl" OR ${PRODUCT} STREQUAL "ascend920emu")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/cloud/slog.conf ${CMAKE_CURRENT_BINARY_DIR}/slog.conf
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/slogd_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend710")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_dc/slog.conf ${CMAKE_CURRENT_BINARY_DIR}/slog.conf
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/slogd_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "helper710")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_dc/slog.conf ${CMAKE_CURRENT_BINARY_DIR}/slog.conf
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/slogd_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "npuf10")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/lhisi/slog.conf ${CMAKE_CURRENT_BINARY_DIR}/slog.conf
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/lhisiLinux/set_up_slogd.sh ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend610")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_mdc/slog.conf ${CMAKE_CURRENT_BINARY_DIR}/slog.conf
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/unzip_tool.py ${CMAKE_CURRENT_BINARY_DIR}
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_mdc/log.yaml ${CMAKE_CURRENT_BINARY_DIR}
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_mdc/slogd.yaml ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend615")
add_custom_target(slogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_mdc/slog.conf ${CMAKE_CURRENT_BINARY_DIR}/slog.conf
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/unzip_tool.py ${CMAKE_CURRENT_BINARY_DIR}
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_mdc/log_pg2.yaml ${CMAKE_CURRENT_BINARY_DIR}/log.yaml
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/1951_mdc/slogd.yaml ${CMAKE_CURRENT_BINARY_DIR}
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

install(TARGETS slogd OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

if (PRODUCT)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/slog.conf OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
if (${PRODUCT} STREQUAL "ascend310" OR ${PRODUCT} STREQUAL "ascend710" OR ${PRODUCT} STREQUAL "ascend910" OR ${PRODUCT} STREQUAL "ascend920" OR ${PRODUCT} STREQUAL "ascend920esl" OR ${PRODUCT} STREQUAL "ascend920emu" OR ${PRODUCT} STREQUAL "helper710")
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/slogd_daemon_monitor.sh OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
elseif (${PRODUCT} STREQUAL "npuf10")
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/set_up_slogd.sh OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend610" OR ${PRODUCT} STREQUAL "ascend615")
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/unzip_tool.py
    ${CMAKE_CURRENT_BINARY_DIR}/log.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/slogd.yaml
    OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
) 
endif ()
endif ()

elseif (${PRODUCT_SIDE} STREQUAL "host")

###################################################################
if (${TARGET_SYSTEM_NAME} STREQUAL "Windows")
add_custom_target(slogHostFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../winconf/slog.conf ${CMAKE_CURRENT_BINARY_DIR}
)
else()
if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend310")
add_custom_target(slogHostFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/mini/slog.conf ${CMAKE_CURRENT_BINARY_DIR}
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/slogd_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
)
elseif (${PRODUCT} STREQUAL "ascend910" OR ${PRODUCT} STREQUAL "ascend920" OR ${PRODUCT} STREQUAL "ascend920esl" OR ${PRODUCT} STREQUAL "ascend920emu")
add_custom_target(slogHostFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/cloud/slog.conf ${CMAKE_CURRENT_BINARY_DIR}
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/slogd_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
)
endif ()
endif ()
endif ()


###################################################################
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR lib/cmake)

include(CMakePackageConfigHelpers)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/slog.conf OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)

if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend310")
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/slogd_daemon_monitor.sh OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
elseif ((${PRODUCT} STREQUAL "ascend910") OR (${PRODUCT} STREQUAL "ascend920") OR (${PRODUCT} STREQUAL "ascend920esl") OR (${PRODUCT} STREQUAL "ascend920emu"))
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/slogd_daemon_monitor.sh
    OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif ()
endif ()

endif ()
