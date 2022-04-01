########################################## alog #######################################
set(ALOG_Src
    ${SLOG_DIR}/slog/slog.cpp
    ${SLOG_DIR}/slog/slog_common.c
    ${SLOG_DIR}/slog/../../shared/cfg_file_parse.c
    ${SLOG_DIR}/slog/../../shared/print_log.c
    ${SLOG_DIR}/slog/../../shared/log_common_util.c
    ${SLOG_DIR}/slog/../../shared/log_level_parse.cpp
    ${SLOG_DIR}/slog/../../shared/msg_queue.c
    ${SLOG_DIR}/slog/../../shared/share_mem.c
    ${SLOG_DIR}/slog/../../shared/log_sys_package.c
)

set(ALOG_Inc 
    ${MMPA_DIR}/inc/mmpa
    ${MMPA_DIR}/inc/mmpa/sub_inc
    ${SECUREC_DIR}/include
)

add_library(mmpa_share SHARED
    ${MMPA_Src}
)

target_include_directories(mmpa_share PRIVATE
    ${MMPA_Inc}
)

target_compile_options(mmpa_share PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(mmpa_share PRIVATE
    OS_TYPE=0
    FUNC_VISIBILITY
)

set_target_properties(mmpa_share
    PROPERTIES
    OUTPUT_NAME mmpa
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mmpa
)

target_link_options(mmpa_share PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)