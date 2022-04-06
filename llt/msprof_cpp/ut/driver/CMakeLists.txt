set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}//dvvp/driver/devmgmt/ai_drv_dev_api.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/driver/channel/ai_drv_prof_api.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
)

set(localSrcFiles
    test/main.cpp 
    ../stub/hdc-log-stub.cpp 
    ../stub/devdrv_runtime_api_stub.cpp 
    ../stub/profile_drv_user_stub.cpp 
    ../stub/common-utils-stub.cpp 
    test/ai_drv_api_utest.cpp 
)

add_executable(driver_utest 
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(driver_utest PRIVATE
    ${TOP_DIR}/abl/libc_sec/include 
    ${TOP_DIR}/inc/driver 
    ${TOP_DIR}/inc/tsch 
    ${TOP_DIR}/abl/adump 
    ${TOP_DIR}/open_source/json/include 
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/metadef/inc
    ${MSPROF_SOURCE_DIR}/dvvp 
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include 
    ${MSPROF_SOURCE_DIR}/dvvp/common 
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc 
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(driver_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(driver_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(driver_utest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(driver_utest
    PROPERTIES
    OUTPUT_NAME driver_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET driver_utest
    TASK_NUM 1
)

