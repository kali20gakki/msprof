if (${PRODUCT_SIDE} STREQUAL "device")
############################# libslog.so ######################################
set(libslogSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/slog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slog_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/cfg_file_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_common_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/msg_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/share_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_sys_package.c)

set(libslogHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${IAM_HEADER_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared)


set (iamLibSlogSrcFiles
     ${CMAKE_CURRENT_SOURCE_DIR}/slog_iam.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/slog_common.c
     ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/msg_queue.c
     ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/cfg_file_parse.c
     ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/print_log.c
     ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_level_parse.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_common_util.c
     ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/share_mem.c
     ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_sys_package.c
     )

set(iamLibSlogIncFiles
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared
    ${IAM_HEADER_DIR}/include
    ${MDC_LINUX_SDK_DIR}/include)

if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend610")
    set(libslogSrcFiles ${iamLibSlogSrcFiles})
    set(libslogHeaderDir ${iamLibSlogIncFiles})
endif ()
endif ()

add_library(slog SHARED
    ${libslogSrcFiles})

target_include_directories(slog PRIVATE
    ${libslogHeaderDir})

target_compile_definitions(slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${SLOG_NAME},alog>:PUBLIC LOG_CPP>
    $<$<STREQUAL:${PRODUCT},ascend610>:IAM LINUX_ETC_CONF>
    $<$<STREQUAL:${PRODUCT},ascend615>:LINUX_ETC_CONF>
    #$<$<STREQUAL:${PRODUCT},ascend615>:IAM>
    WRITE_TO_SYSLOG
)

target_compile_options(slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -O2
    -fPIC
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_directories(slog PRIVATE
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
)

target_link_libraries(slog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${PRODUCT},ascend610>:iam>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend610>:easy_comm>
    $<$<STREQUAL:${PRODUCT},ascend615>:iam>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend615>:easy_comm>
    c_sec
)

if (SLOG_NAME)
if (${SLOG_NAME} STREQUAL "alog")
set_target_properties(slog
    PROPERTIES
    OUTPUT_NAME alog
)
else ()
set_target_properties(slog
    PROPERTIES
    OUTPUT_NAME slog
)
endif ()
else ()
set_target_properties(slog
    PROPERTIES
    OUTPUT_NAME slog
)
endif()

############################# libalog.so ######################################
add_library(alog SHARED
    ${libslogSrcFiles})

target_include_directories(alog PRIVATE
    ${libslogHeaderDir})

target_compile_definitions(alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${PRODUCT},ascend610>:IAM LINUX_ETC_CONF>
    $<$<STREQUAL:${PRODUCT},ascend615>:LINUX_ETC_CONF>
    #$<$<STREQUAL:${PRODUCT},ascend615>:IAM>
    PUBLIC LOG_CPP
    WRITE_TO_SYSLOG
)

target_compile_options(alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -O2
    -fPIC
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_directories(alog PRIVATE
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
)

target_link_libraries(alog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${PRODUCT},ascend610>:iam>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend610>:easy_comm>
    $<$<STREQUAL:${PRODUCT},ascend615>:iam>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend615>:easy_comm>
    c_sec
)

set_target_properties(alog
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY alog
    OUTPUT_NAME alog
)

############################# alog.a #######################################
add_library(static_alog STATIC
    ${libslogSrcFiles})

target_include_directories(static_alog PRIVATE
    ${libslogHeaderDir})

target_compile_definitions(static_alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${PRODUCT},ascend610>:IAM LINUX_ETC_CONF>
    $<$<STREQUAL:${PRODUCT},ascend615>:LINUX_ETC_CONF>
    #$<$<STREQUAL:${PRODUCT},ascend615>:IAM>
    PUBLIC LOG_CPP
    WRITE_TO_SYSLOG
)

target_compile_options(static_alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-O2>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(static_alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_directories(static_alog PRIVATE
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
)

target_link_libraries(static_alog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
)

set_target_properties(static_alog
    PROPERTIES
    OUTPUT_NAME alog>
)

############################# slog.a ######################################
set(static_libslogSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/slog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slog_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/cfg_file_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_common_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/msg_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/share_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_sys_package.c)

set(static_libslogHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared)

if (PRODUCT)
if (${PRODUCT} STREQUAL "ascend610")
    set(static_libslogSrcFiles ${iamLibSlogSrcFiles})
    set(static_libslogHeaderDir ${iamLibSlogIncFiles})
endif ()
endif ()

add_library(static_slog STATIC
    ${static_libslogSrcFiles})

target_include_directories(static_slog PRIVATE
    ${static_libslogHeaderDir})

target_compile_definitions(static_slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${PRODUCT},ascend610>:IAM LINUX_ETC_CONF>
    $<$<STREQUAL:${PRODUCT},ascend615>:LINUX_ETC_CONF>
    #$<$<STREQUAL:${PRODUCT},ascend615>:IAM>
)

target_compile_options(static_slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -O2
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(static_slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_directories(static_slog PRIVATE
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend610>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64>
    $<$<STREQUAL:${PRODUCT},ascend615>:${MDC_LINUX_SDK_DIR}/lib64/mdc/base-plat>
)

target_link_libraries(static_slog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${PRODUCT},ascend610>:iam>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend610>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend610>:easy_comm>
    $<$<STREQUAL:${PRODUCT},ascend615>:iam>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lrt>
    $<$<STREQUAL:${PRODUCT},ascend615>:-lxshmem>
    $<$<STREQUAL:${PRODUCT},ascend615>:easy_comm>
    c_sec_static
)

set_target_properties(static_slog
    PROPERTIES
    OUTPUT_NAME slog
)

###################################################################
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR  lib/cmake)

include(CMakePackageConfigHelpers)

install(TARGETS slog alog static_slog OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

elseif (${PRODUCT_SIDE} STREQUAL "host")
############################# libalog.so ######################################
set(libalogSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/slog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slog_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/cfg_file_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_common_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_sys_package.c
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/../../shared/share_mem.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../plog/process_log.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../shared/external/log_drv.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../shared/external/log_runtime.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../shared/external/api/driver_api.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../shared/library_load.c>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_to_file.c>
)

set(libalogHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}/../../shared/external>
    ${CMAKE_CURRENT_SOURCE_DIR}/../plog
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${CMAKE_CURRENT_SOURCE_DIR}>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:${TOP_DIR}/inc/driver>
)

add_library(slog SHARED
    ${libalogSrcFiles})

target_include_directories(slog PRIVATE
    ${libalogHeaderDir})

target_compile_definitions(slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:WRITE_TO_SYSLOG>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:HAL_REGISTER_ALOG>
    PUBLIC LOG_CPP
    PROCESS_LOG
)

target_compile_options(slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-O2>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -fPIC
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_libraries(slog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:$<BUILD_INTERFACE:adump_headers>>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:$<BUILD_INTERFACE:mmpa_headers>>
    c_sec
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:mmpa>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,--whole-archive>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:static_plog>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,--no-whole-archive>
)

set_target_properties(slog
    PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,libalog,alog>
)

####################################################################
add_library(alog SHARED
    ${libalogSrcFiles})

add_dependencies(slog alog)

target_include_directories(alog PRIVATE
    ${libalogHeaderDir})

target_compile_definitions(alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:WRITE_TO_SYSLOG>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:HAL_REGISTER_ALOG>
    PUBLIC LOG_CPP
    PROCESS_LOG
)

target_compile_options(alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-O2>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -fPIC
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_libraries(alog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:$<BUILD_INTERFACE:adump_headers>>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:$<BUILD_INTERFACE:mmpa_headers>>
    c_sec
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:mmpa>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,--whole-archive>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:static_plog>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,--no-whole-archive>
)

set_target_properties(alog
    PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
    LIBRARY_OUTPUT_DIRECTORY alog
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,libalog,alog>
)

############################# alog.a #######################################
add_library(static_alog STATIC
    ${libalogSrcFiles})

target_include_directories(static_alog PRIVATE
    ${libalogHeaderDir})

target_compile_definitions(static_alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:WRITE_TO_SYSLOG>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:LARGE_LOGFILE_VOLUME>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:HAL_REGISTER_ALOG>
    PUBLIC LOG_CPP
    PROCESS_LOG
)

target_compile_options(static_alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-O2>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(static_alog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_libraries(static_alog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:$<BUILD_INTERFACE:adump_headers>>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:$<BUILD_INTERFACE:mmpa_headers>>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,--whole-archive>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:static_plog>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,--no-whole-archive>
)

set_target_properties(static_alog
    PROPERTIES
    OUTPUT_NAME $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,static_libalog,alog>
)

############################# slog.a ######################################
set(static_libslogSrcFiles
    ${CMAKE_CURRENT_SOURCE_DIR}/slog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slog_common.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/cfg_file_parse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/print_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_common_util.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_level_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/msg_queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/share_mem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/log_sys_package.c)

set(static_libslogHeaderDir
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared)

add_library(static_slog STATIC
    ${static_libslogSrcFiles})

target_include_directories(static_slog PRIVATE
    ${static_libslogHeaderDir})

target_compile_definitions(static_slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE_DEF=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE_DEF=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:WRITE_TO_SYSLOG>
)

target_compile_options(static_slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-O2>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal -Wextra>
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

target_link_options(static_slog PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wl,-z,relro,-z,now,-z,noexecstack>
)

target_link_libraries(static_slog PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:slog_headers>
    c_sec_static
)

set_target_properties(static_slog
    PROPERTIES
    OUTPUT_NAME slog
)

###################################################################
set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_RUNTIME_DIR bin)
set(INSTALL_CONFIG_DIR lib/cmake)

include(CMakePackageConfigHelpers)

install(TARGETS slog alog static_slog static_alog OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

if (${TARGET_SYSTEM_NAME} STREQUAL "Windows")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/libslog.pdb OPTIONAL
    DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif()

endif()
