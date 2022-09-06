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
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger/cmd_log.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/slog_plugin.cpp
    # ${MSPROF_SOURCE_DIR}/dvvp/depend/src/driver_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp    
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp        
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp        
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_interface.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_mini.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_mdc.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_lhisi.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_dc.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_cloudv2.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_cloud.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/mmpa/mmpa_api.cpp
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
    ${MSPROF_SOURCE_DIR}/dvvp/adx/stub/adx_prof_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/slog_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_handle.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_impl.cpp
    ${TOP_DIR}/collector/dvvp/params_adapter/params_adapter.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_cloud.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_cloudv2.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_dc.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_interface.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_lhisi.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_mdc.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_mini.cpp   
)


set(src_file
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/msprof_bin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger/cmd_log.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/msprof_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/prof_params_adapter.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/data_handle.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/input_parser.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/prof_task.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/running_mode.cpp
)

add_executable(msprof_bin_utest
    ${common_file}
    ${src_file}
    test/main.cpp
    test/msprof_bin_utest.cpp
    test/input_parser_utest.cpp
    test/data_handle_utest.cpp
    test/msprof_manager_utest.cpp
    test/prof_param_adapter_utest.cpp
    test/prof_task_utest.cpp
    test/prof_device_transport_utest.cpp
    test/running_mode_utest.cpp
    ../stub/profile_drv_user_stub.cpp
    ../stub/hdc-api-stub.cpp
    ../stub/hdc-log-stub.cpp
    ../stub/ide-cmd-stub.cpp
    ../stub/devdrv_runtime_api_stub.cpp
    ../../stub/driver_plugin_stub.cpp
    ${CppPath}
)

target_include_directories(msprof_bin_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/collector/inc/toolchain
    ${TOP_DIR}/collector/inc/external
    ${TOP_DIR}/collector/dvvp/depend/inc/acl
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/opensource/json/include
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/../inc
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/include
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin
    ${MSPROF_SOURCE_DIR}/dvvp/app
    ${MSPROF_SOURCE_DIR}/dvvp/
    ${MSPROF_SOURCE_DIR}/dvvp/host/include/
    ${MSPROF_SOURCE_DIR}/dvvp/common
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/
    ${MSPROF_SOURCE_DIR}/dvvp/common/mmpa
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils 
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils
    ${MSPROF_SOURCE_DIR}/inc/toolchain
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include
    ${MSPROF_SOURCE_DIR}/dvvp/transport
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/stub/
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
    ${MSPROF_SOURCE_DIR}/dvvp
)


target_compile_options(msprof_bin_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
    -fprofile-arcs
    -ftest-coverage
)

target_compile_definitions(msprof_bin_utest PRIVATE
    C_SEC
    __PROF_UT
    google=ascend_private
)

target_link_libraries(msprof_bin_utest PRIVATE
    ascend_protobuf_static
    c_sec_static
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

target_link_options(msprof_bin_utest PRIVATE
	-lgcov
	--coverage
)

set_target_properties(msprof_bin_utest
    PROPERTIES
    OUTPUT_NAME msprof_bin_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET msprof_bin_utest
    TASK_NUM 1
)
