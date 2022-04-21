
add_executable(stub_file_utest
    ${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/hdc_api.cpp  
    ${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/memory_utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/wrapper/error_manager/msprof_error_manager.cpp
    test/main.cpp
    test/hdc_api_utest.cpp
    test/msprof_err_mgr_utest.cpp
    ..//stub/mmpa_stub.cpp
    ../stub/profile_drv_user_stub.cpp
    ../stub/hdc-log-stub.cpp
    ../stub/devdrv_runtime_api_stub.cpp
    ../stub/ide_hdc_stub.cc
    ../stub/error_manager_stub.cpp

)

target_include_directories(stub_file_utest PRIVATE
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/metadef/inc
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/common/
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/wrapper
    ${MSPROF_SOURCE_DIR}/dvvp
)


target_compile_options(stub_file_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra

)

target_compile_definitions(stub_file_utest PRIVATE
    C_SEC
    __PROF_UT
    google=ascend_private
)

target_link_libraries(stub_file_utest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:adump_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(stub_file_utest
    PROPERTIES
    OUTPUT_NAME stub_file_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET stub_file_utest
    TASK_NUM 1
)
