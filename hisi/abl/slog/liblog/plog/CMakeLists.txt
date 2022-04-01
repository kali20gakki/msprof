###################################################################
set(plogSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/process_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/external/log_drv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/external/log_runtime.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/external/api/driver_api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/library_load.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_to_file.c
)

set(plogHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/abl/slog/shared/external
    ${TOP_DIR}/abl/slog/shared
    ${TOP_DIR}/abl/slog/liblog/slog
)

add_library(static_plog STATIC
    ${plogSrcFiles})

target_include_directories(static_plog PRIVATE
    ${plogHeaderDir})

target_compile_definitions(static_plog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:WRITE_TO_SYSLOG>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:HAL_REGISTER_ALOG>
    PROCESS_LOG
)

target_link_options(static_plog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_compile_options(static_plog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-O2>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_libraries(static_plog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:adump_headers>
    c_sec_static
    static_mmpa
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:dl>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:rt>
)

set_target_properties(static_plog
    PROPERTIES
    OUTPUT_NAME plog
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

###################################################################
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR lib/cmake)

include(CMakePackageConfigHelpers)

install(TARGETS static_plog OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

