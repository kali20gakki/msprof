###################################################################
set(sklogdSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/klogd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../slog/src/log_monitor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/log_sys_package.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/start_single_process.c)

set(sklogdHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../slog/src)

add_executable(sklogd
    ${sklogdSrcFiles})

target_include_directories(sklogd PRIVATE
    ${sklogdHeaderDir})

target_compile_definitions(sklogd PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${PRODUCT},ascend310>:_LOG_MONITOR_>
    $<$<STREQUAL:${PRODUCT},ascend910>:_LOG_MONITOR_>
    $<$<STREQUAL:${PRODUCT},ascend920>:_LOG_MONITOR_>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:_LOG_MONITOR_>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:_LOG_MONITOR_>
    $<$<STREQUAL:${PRODUCT},ascend710>:_LOG_MONITOR_>
    $<$<STREQUAL:${PRODUCT},helper710>:_LOG_MONITOR_>
    _SLOG_DEVICE_
)

target_compile_options(sklogd PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -O2
    -Werror
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(sklogd PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_libraries(sklogd PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${PRODUCT},ascend310>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend910>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},ascend710>:ascend_monitor>
    $<$<STREQUAL:${PRODUCT},helper710>:ascend_monitor>
    c_sec
    slog
)

set_target_properties(sklogd
    PROPERTIES
    OUTPUT_NAME sklogd
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

###################################################################
if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend310" OR ${PRODUCT} STREQUAL "ascend710" OR ${PRODUCT} STREQUAL "ascend910" OR ${PRODUCT} STREQUAL "ascend920" OR ${PRODUCT} STREQUAL "ascend920esl" OR ${PRODUCT} STREQUAL "ascend920emu" OR ${PRODUCT} STREQUAL "helper710")
add_custom_target(sklogDeviceFiles
     COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../../../scripts/sklogd_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
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

install(TARGETS sklogd OPTIONAL
    EXPORT sklog_targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend310" OR ${PRODUCT} STREQUAL "ascend710" OR ${PRODUCT} STREQUAL "ascend910" OR ${PRODUCT} STREQUAL "ascend920" OR ${PRODUCT} STREQUAL "ascend920esl" OR ${PRODUCT} STREQUAL "ascend920emu" OR ${PRODUCT} STREQUAL "helper710")
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/sklogd_daemon_monitor.sh OPTIONAL DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif ()
endif ()
