protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

################ msprofiler_utest ################
set(profilerSrcList
	${MSPROF_SOURCE_DIR}/dvvp/adx/stub/adx_prof_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/memory_utils.cpp
	${MSPROF_SOURCE_DIR}/dvvp/analyze/src/analyzer.cpp
	${MSPROF_SOURCE_DIR}/dvvp/analyze/src/analyzer_base.cpp
	${MSPROF_SOURCE_DIR}/dvvp/analyze/src/analyzer_ge.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/analyze/src/analyzer_ffts.cpp
	${MSPROF_SOURCE_DIR}/dvvp/analyze/src/analyzer_hwts.cpp
	${MSPROF_SOURCE_DIR}/dvvp/analyze/src/analyzer_ts.cpp
	${MSPROF_SOURCE_DIR}/dvvp/analyze/src/op_desc_parser.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger/cmd_log.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/slog_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp	
	${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp        
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp        
	${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/mmpa/mmpa_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
	${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
	${MSPROF_SOURCE_DIR}/dvvp/depend/src/slog_plugin.cpp
	${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_handle.cpp
	${MSPROF_SOURCE_DIR}/dvvp/driver/channel/ai_drv_prof_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/mini/ai_drv_dsmi_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/collection_register.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/job_device_soc.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/job_factory.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_channel_manager.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_job.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_peripheral_job.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_timer.cpp
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/prof_host_job.cpp
	${MSPROF_SOURCE_DIR}/dvvp/message/codec.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/common/src/msprof_callback_handler.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/common/src/receive_data.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/aicpu_plugin.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/prof_acl_mgr.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/prof_ge_core.cpp
	${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_acljson.cpp
	${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_geopt.cpp
	${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_aclapi.cpp
	${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/prof_acl_core.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/prof_api_common.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/uploader_dumper.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/platform/mini/data_dumper.cpp
	${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_cloud.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_cloudv2.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_dc.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_interface.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_lhisi.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_mdc.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform_adapter_mini.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_tx_manager.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_tx_reporter.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_stamp_pool.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/command_handle.cpp
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/msprof_callback_impl.cpp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/device.cpp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/info_json.cpp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/prof_manager.cpp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/prof_params_adapter.cpp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/prof_task.cpp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/task_relationship_mgr.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/file_slice.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/file_ageing.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/file_transport.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/hdc_transport.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/platform/soc/dev_mgr_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/parser_transport.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/prof_channel.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/transport.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/uploader.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/uploader_mgr.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/hash_data.cpp
	${CppPath}
)

set(profilerSrcFile
    test/main.cpp
    test/msprof_callback_handler_utest.cpp
    test/prof_acl_core_utest.cpp
    test/aicpu_plugin_utest.cpp
    ../stub/ge_stub.cpp
    ../stub/hdc-log-stub.cpp
    ../stub/devdrv_runtime_api_stub.cpp
    ../stub/profile_drv_user_stub.cpp
    ../stub/hdc-api-stub.cpp
    ../../stub/driver_plugin_stub.cpp
    ${profilerSrcList}
)


add_executable(
    msprofiler_utest
    ${profilerSrcFile}
)

target_include_directories(msprofiler_utest PRIVATE
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils 
    ${TOP_DIR}/abl/msprof/inc/toolchain
	../stub/
	${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/collector/inc/
    ${TOP_DIR}/collector/inc/toolchain/
    ${TOP_DIR}/collector/inc/external/
    ${TOP_DIR}/collector/inc/external/acl
    ${TOP_DIR}/collector/inc/external/ge
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/adump
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/../inc
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/graphengine/external
	${MSPROF_SOURCE_DIR}/../inc/toolchain
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/acl
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
	${MSPROF_SOURCE_DIR}/dvvp/analyze/inc
	${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/common/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/drvprof/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/platform/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/include
	${MSPROF_SOURCE_DIR}/dvvp/common
	${MSPROF_SOURCE_DIR}/dvvp/common/errno
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/
	${MSPROF_SOURCE_DIR}/dvvp/common/validation
	${MSPROF_SOURCE_DIR}/dvvp/common/queue
	${MSPROF_SOURCE_DIR}/dvvp/common/singleton
	${MSPROF_SOURCE_DIR}/dvvp/common/utils
	${MSPROF_SOURCE_DIR}/dvvp/common/config
	${MSPROF_SOURCE_DIR}/dvvp/common/mmpa
	${MSPROF_SOURCE_DIR}/dvvp/message
	${MSPROF_SOURCE_DIR}/dvvp/transport
	${MSPROF_SOURCE_DIR}/dvvp/common/memory/
	${MSPROF_SOURCE_DIR}/dvvp/streamio/client/include
	${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include
	${MSPROF_SOURCE_DIR}/dvvp/streamio/server/include
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform
	${MSPROF_SOURCE_DIR}/dvvp
	${MSPROF_SOURCE_DIR}/dvvp/task_handle/include
	${MSPROF_SOURCE_DIR}/dvvp/driver/inc
	${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include
	${MSPROF_SOURCE_DIR}/dvvp/adx/inc	
	${TOP_DIR}/graphengine/inc/external
	${TOP_DIR}/inc
	${TOP_DIR}/inc/driver
	${TOP_DIR}/inc/external
	${TOP_DIR}/inc/external/acl
    ${TOP_DIR}/collector/dvvp/depend/inc/acl
    ${TOP_DIR}/collector/dvvp/depend/inc/profapi
	${TOP_DIR}/ace/npuruntime/inc
	${TOP_DIR}/abl/msprof/inc
	${MSPROF_SOURCE_DIR}/dvvp/msprofiler/inc
    ${CMAKE_BINARY_DIR}/proto/profiler/
	${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)


target_compile_options(msprofiler_utest PRIVATE
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

target_compile_definitions(msprofiler_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_options(msprofiler_utest PRIVATE
	-lgcov
	--coverage
)

target_link_libraries(msprofiler_utest PRIVATE
	ascend_protobuf_static
    c_sec_static
	pthread
	${GTEST_STATIC_LIBRARY}
	${MOCKCPP_STATIC_LIBRARY}
	dl
)

set_target_properties(msprofiler_utest
    PROPERTIES
    OUTPUT_NAME msprofiler_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET msprofiler_utest
    TASK_NUM 1
)

# msprof utest
add_executable(msprof_utest
	test/main.cpp
	test/msprof_utest.cpp
	../stub/hdc-log-stub.cpp
	../stub/devdrv_runtime_api_stub.cpp
	../stub/profile_drv_user_stub.cpp
	../stub/hdc-api-stub.cpp
	../stub/ide-cmd-stub.cpp
	../../stub/driver_plugin_stub.cpp
	${MSPROF_SOURCE_DIR}/dvvp/adx/stub/adx_prof_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger/cmd_log.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/slog_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp	
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp        
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/mmpa/mmpa_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
	${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp
	${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/message/codec.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/common/src/receive_data.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_tx_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_tx_reporter.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_stamp_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/platform/mini/data_dumper.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/client/src/sender.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/hdc_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/uploader.cpp
	${MSPROF_SOURCE_DIR}/dvvp/transport/uploader_mgr.cpp
	${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_handle.cpp
	${CppPath}
)

target_include_directories(msprof_utest PRIVATE
	${GTEST_INCLUDE_DIR}
	${MOCKCPP_INCLUDE_DIR}
	${TOP_DIR}/platform/securec/include
	${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/collector/inc/
    ${TOP_DIR}/collector/inc/toolchain/
    ${TOP_DIR}/collector/inc/external/
    ${TOP_DIR}/collector/inc/external/acl
    ${TOP_DIR}/collector/inc/external/ge
    ${TOP_DIR}/collector/dvvp/depend/inc/acl
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/adump
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc
	${MSPROF_SOURCE_DIR}/../inc
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/graphengine/external
	${MSPROF_SOURCE_DIR}/../inc/toolchain
	${MSPROF_SOURCE_DIR}/dvvp/msprof/
	${MSPROF_SOURCE_DIR}/dvvp/msprof/drvprof
	${MSPROF_SOURCE_DIR}/dvvp/msprof/drvprof/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/common/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/platform/include
	${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/include
	${MSPROF_SOURCE_DIR}/dvvp/common
	${MSPROF_SOURCE_DIR}/dvvp/common/mmpa
	${MSPROF_SOURCE_DIR}/dvvp/common/validation
	${MSPROF_SOURCE_DIR}/dvvp/common/errno
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/
	${MSPROF_SOURCE_DIR}/dvvp/common/queue
	${MSPROF_SOURCE_DIR}/dvvp/common/singleton
	${MSPROF_SOURCE_DIR}/dvvp/common/utils
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
	${MSPROF_SOURCE_DIR}/dvvp/common/config
	${MSPROF_SOURCE_DIR}/dvvp/common/errno
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform
    ${MSPROF_SOURCE_DIR}/dvvp/message
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
	${MSPROF_SOURCE_DIR}/dvvp/message
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
	${MSPROF_SOURCE_DIR}/dvvp/transport
	${MSPROF_SOURCE_DIR}/dvvp/adx/inc
	${MSPROF_SOURCE_DIR}/dvvp/streamio/client/include
	${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include
	${MSPROF_SOURCE_DIR}/dvvp/./
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
	../stub
	${CMAKE_BINARY_DIR}/proto/profiler
	${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(msprof_utest PRIVATE
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

target_link_options(msprof_utest PRIVATE
	-lgcov
	--coverage
)

target_compile_definitions(msprof_utest PRIVATE
    C_SEC
    MSPROF
    ADX_LIB
    google=ascend_private
)

target_link_libraries(msprof_utest PRIVATE
	ascend_protobuf_static
    c_sec_static
	pthread
	${GTEST_STATIC_LIBRARY}
	${MOCKCPP_STATIC_LIBRARY}
	dl
)

set_target_properties(msprof_utest
    PROPERTIES
    OUTPUT_NAME msprof_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET msprof_utest
    TASK_NUM 1
)
