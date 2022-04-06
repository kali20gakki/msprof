protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)
set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/common/src/select_operation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_slice.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_ageing.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${CppPath}
)

set(localSrcFiles
	test/main.cpp 
	test/select_operation_test.cpp 
	test/file_slice_test.cpp 
	test/file_ageing_test.cpp
	../../stub/common-utils-stub.cpp 
	../../stub/hdc-log-stub.cpp
    ../../stub/devdrv_runtime_api_stub.cpp
)

add_executable(streamio_common_utest
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(streamio_common_utest PRIVATE
	${TOP_DIR}/abl/adump/inc/adump
	${TOP_DIR}/abl/libc_sec/include    
	${TOP_DIR}/inc/driver 
	${TOP_DIR}/open_source/json/include 
	${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/metadef/inc
	${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include 
	${MSPROF_SOURCE_DIR}/dvvp/ 
	${MSPROF_SOURCE_DIR}/dvvp/common 
	${MSPROF_SOURCE_DIR}/dvvp/common/config
	${MSPROF_SOURCE_DIR}/dvvp/common/logger
	${MSPROF_SOURCE_DIR}/dvvp/common/validation 
	${MSPROF_SOURCE_DIR}/dvvp/aging 
	${MSPROF_SOURCE_DIR}/dvvp/transport 
	${MSPROF_SOURCE_DIR}/dvvp/common/statistics
	${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${TOP_DIR}/abl/libc_sec/include 
    ${TOP_DIR}/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(streamio_common_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(streamio_common_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(streamio_common_utest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(streamio_common_utest
    PROPERTIES
    OUTPUT_NAME streamio_common_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET streamio_common_utest
    TASK_NUM 1
)

