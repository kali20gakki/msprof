###################################################################
set(reportSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/msnpureport.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_level_parse.cpp
)

set(reportHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/abl/adump/external
    ${TOP_DIR}/abl/adump/hdc-common
    ${TOP_DIR}/abl/slog/shared
)

set(msnpureport_auto_export
    ${TOP_DIR}/abl/slog/syslog/scripts/msnpureport_auto_export.sh
)

add_executable(msnpureport
    ${reportSrcFiles})

target_include_directories(msnpureport PRIVATE
    ${reportHeaderDir})

target_compile_definitions(msnpureport PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
)

target_compile_options(msnpureport PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-O2>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -fPIE
    -fstack-protector-strong
    -fvisibility=hidden
)

target_link_options(msnpureport PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_libraries(msnpureport PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    ascend_hal
    c_sec_static
    static_mmpa
    adcore
    bbox_dump
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:static_slog>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:dl>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:rt>
)

set_target_properties(msnpureport
    PROPERTIES
    OUTPUT_NAME msnpureport
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

###################################################################
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR lib/cmake)

include(CMakePackageConfigHelpers)

install(TARGETS msnpureport OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

install(FILES ${msnpureport_auto_export} OPTIONAL
    DESTINATION ${INSTALL_LIBRARY_DIR}
)

