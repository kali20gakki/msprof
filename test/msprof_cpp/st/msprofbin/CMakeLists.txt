protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

set(common_file
    ${MSPROF_SOURCE_DIR}/dvvp/app/application.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/app/env_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dsmi_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/channel/ai_drv_prof_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/collection_register.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/job_factory.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/job_device_rpc.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_job.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_peripheral_job.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_timer.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_host_job.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_channel_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/job_device_soc.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/message/codec.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/prof_channel.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/uploader_mgr.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/uploader.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_slice.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_ageing.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/info_json.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/task_relationship_mgr.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/hdc_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/device_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/platform/rpc/dev_mgr_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/adx/stub_lhisi/adx_prof_api.cpp   
    
)


set(src_file
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/msprof_bin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/cmd_log.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/msprof_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/prof_params_adapter.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/data_handle.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/input_parser.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/prof_task.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/running_mode.cpp
    
)

add_executable(msprof_bin_stest
    ${common_file}
    ${src_file}
    test/main.cpp
    test/msprof_bin_stest.cpp
    test/input_parser_stest.cpp
    test/data_handle_stest.cpp
    test/msprof_manager_stest.cpp
    test/prof_param_adapter_stest.cpp
    test/prof_task_stest.cpp
    test/prof_device_transport_stest.cpp
    test/running_mode_stest.cpp
    ..//stub/mmpa_stub.cpp
    ../stub/profile_drv_user_stub.cpp
    ../stub/hdc-api-stub.cpp
    ../stub/hdc-log-stub.cpp
	../stub/devdrv_runtime_api_stub.cpp
    ${CppPath}
)

target_include_directories(msprof_bin_stest PRIVATE
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/include
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin
    ${MSPROF_SOURCE_DIR}/dvvp/app
    ${MSPROF_SOURCE_DIR}/dvvp/
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/open_source/json/include
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/metadef/inc
    ${MSPROF_SOURCE_DIR}/dvvp/host/include/
    ${MSPROF_SOURCE_DIR}/dvvp/common
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include
    ${MSPROF_SOURCE_DIR}/dvvp/transport
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/stub/
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)


target_compile_options(msprof_bin_stest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(msprof_bin_stest PRIVATE
    C_SEC
    __PROF_UT
    google=ascend_private
)

target_link_libraries(msprof_bin_stest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:adump_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(msprof_bin_stest
    PROPERTIES
    OUTPUT_NAME msprof_bin_stest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET msprof_bin_stest
    TASK_NUM 1
)
