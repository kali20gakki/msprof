protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

set(profilerSrcList
	${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/adx_prof_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
	${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
	${MSPROF_SOURCE_DIR}/dvvp/device/src/collect_engine.cpp
	${MSPROF_SOURCE_DIR}/dvvp/device/src/collection_entry
	${MSPROF_SOURCE_DIR}/dvvp/device/src/prof_device_core.cpp
	${MSPROF_SOURCE_DIR}/dvvp/device/src/prof_job_handler.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/device/src/prof_msg_handler.cpp
	${MSPROF_SOURCE_DIR}/dvvp/device/src/task_manager.cpp
	${MSPROF_SOURCE_DIR}/dvvp/driver/channel/ai_drv_prof_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/collection_register.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_channel_manager.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_job.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_peripheral_job.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_timer.cpp
	${MSPROF_SOURCE_DIR}/dvvp/message/codec.cpp
	${MSPROF_SOURCE_DIR}/dvvp/message/dispatcher.cpp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/task_relationship_mgr.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/file_slice.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/file_ageing.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/hdc_transport.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/prof_channel.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/receiver.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/transport.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/uploader.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/uploader_mgr.cpp
    ${CppPath}
)

set(profilerSrcFile
    testcase/main.cpp
	testcase/prof_device_core_stest.cpp
	testcase/prof_timer_stest.cpp
	../stub/hdc-api-stub.cpp
	../stub/hdc-log-stub.cpp
	../stub/devdrv_runtime_api_stub.cpp
	../stub/profile_drv_user_stub.cpp
	../stub/mmpa_stub.cpp
    ${profilerSrcList}
)


add_executable(
    device_stest
    ${profilerSrcFile}
)

target_include_directories(device_stest PRIVATE
	${TOP_DIR}/abl/libc_sec/include
	${TOP_DIR}/inc/driver
	${TOP_DIR}/inc/tsch
	${TOP_DIR}/abl/adump
	${TOP_DIR}/abl/adump/external
	${TOP_DIR}/open_source/json/include
	${TOP_DIR}/ace/npuruntime/inc
	${TOP_DIR}/abl/msprof/inc
	${TOP_DIR}/metadef/inc
	${MSPROF_SOURCE_DIR}/dvvp
	${MSPROF_SOURCE_DIR}/dvvp/adx/inc
	${MSPROF_SOURCE_DIR}/dvvp/common
	${MSPROF_SOURCE_DIR}/dvvp/common/logger
	${MSPROF_SOURCE_DIR}/dvvp/common/platform
	${MSPROF_SOURCE_DIR}/dvvp/common/statistics
	${MSPROF_SOURCE_DIR}/dvvp/common/validation
	${MSPROF_SOURCE_DIR}/dvvp/device/include
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include
	${MSPROF_SOURCE_DIR}/dvvp/transport
	${MSPROF_SOURCE_DIR}/dvvp/driver/inc
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/include
	${MSPROF_SOURCE_DIR}/dvvp/streamio/server/include
	${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include
	../stub
    ${CMAKE_BINARY_DIR}/proto/profiler/
	${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)


target_compile_options(device_stest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(device_stest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_options(device_stest PRIVATE
   -ldl
   -lpthread
)

target_link_libraries(device_stest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:adump_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(device_stest
    PROPERTIES
    OUTPUT_NAME device_stest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET device_stest
    TASK_NUM 1
)
