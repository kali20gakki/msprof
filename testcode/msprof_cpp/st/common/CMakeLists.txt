protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

set(msprofSrcList
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/channel/ai_drv_prof_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/message/codec.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${CppPath}
)

set(testCfgSrcFile
    ../stub/common-utils-stub.cpp
    ../stub/devdrv_runtime_api_stub.cpp
    ../stub/profile_drv_user_stub.cpp
    ../stub/hdc-log-stub.cpp
    testcase/main.cpp
	testcase/config_manager_stest.cpp
    ${msprofSrcList}
)

add_executable(
    common_stest
    ${testCfgSrcFile}
)

target_include_directories(common_stest PRIVATE
    ../
    ../stub
    ${TOP_DIR}/llt/abl/msprof/msprof_cpp/ut/stub
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/abl/adump
    ${TOP_DIR}/open_source/json/include
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/metadef/inc
    ${MSPROF_SOURCE_DIR}/dvvp
    ${MSPROF_SOURCE_DIR}/dvvp/common
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${MSPROF_SOURCE_DIR}/dvvp/common/errno
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(common_stest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(common_stest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_options(common_stest PRIVATE
   -ldl
   -lpthread
)

target_link_libraries(common_stest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:adump_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(common_stest
    PROPERTIES
    OUTPUT_NAME common_stest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET common_stest
    TASK_NUM 1
)
