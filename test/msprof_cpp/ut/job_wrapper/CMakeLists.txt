protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

set(profilerSrcFiles
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
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
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
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src/uploader_dumper.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/platform/mini/data_dumper.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_tx_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/msprof_tx_reporter.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src/prof_stamp_pool.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofiler/src/command_handle.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofiler/src/msprof_callback_impl.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/device.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/info_json.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/prof_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/prof_params_adapter.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/prof_task.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/src/task_relationship_mgr.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_slice.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_ageing.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hash_data.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/hdc_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/platform/soc/dev_mgr_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/parser_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/prof_channel.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/uploader.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/uploader_mgr.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/slog_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_handle.cpp
    ${CppPath}
)

set(localSrcFiles
    test/main.cpp
    ../stub/ge_stub.cpp
    ../stub/hdc-api-stub.cpp
    ../stub/hdc-log-stub.cpp
    ../stub/common-utils-stub.cpp
    ../stub/devdrv_runtime_api_stub.cpp
    ../stub/profile_drv_user_stub.cpp
    ../../stub/driver_plugin_stub.cpp
    test/prof_job_utest.cpp
    test/prof_peripheral_job_utest.cpp
    test/prof_timer_utest.cpp
    test/collection_register_utest.cpp
    test/prof_device_soc_utest.cpp
    test/job_factory_utest.cpp
    test/prof_channel_manager_utest.cpp
    test/prof_host_job_utest.cpp
    test/adx_prof_api_utest.cpp
)


add_executable(job_wrapper_utest
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_compile_definitions(job_wrapper_utest PRIVATE
    C_SEC
)


target_include_directories(job_wrapper_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/inc/toolchain
    ${TOP_DIR}/collector/inc/
    ${TOP_DIR}/collector/inc/toolchain
    ${TOP_DIR}/collector/inc/external
    ${TOP_DIR}/collector/inc/external/acl
    ${TOP_DIR}/collector/inc/external/ge
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/adump
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/mmpa
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/acl
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/graphengine/external
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/../inc
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils 
    ${TOP_DIR}/abl/msprof/inc/toolchain
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/open_source/json/include
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/metadef/inc/external
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
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${MSPROF_SOURCE_DIR}/dvvp/common/queue
    ${MSPROF_SOURCE_DIR}/dvvp/common/singleton
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${MSPROF_SOURCE_DIR}/dvvp/message
    ${MSPROF_SOURCE_DIR}/dvvp/transport
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/client/include
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/server/include
    ${MSPROF_SOURCE_DIR}/dvvp
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${TOP_DIR}/graphengine/inc/external
    # ${TOP_DIR}/graphengine/inc/framework/common
    ${TOP_DIR}/inc
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/inc/external
    ${TOP_DIR}/inc/external/acl
    ${TOP_DIR}/ace/npuruntime/inc
    ${MSPROF_SOURCE_DIR}/dvvp/msprofiler/inc
    ../stub/
    ${TOP_DIR}/llt/abl/msprof/msprof_cpp/st/stub 
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(job_wrapper_utest PRIVATE
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

target_link_options(job_wrapper_utest PRIVATE
	-lgcov
	--coverage
)

target_compile_definitions(job_wrapper_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(job_wrapper_utest PRIVATE
    ascend_protobuf_static
    c_sec_static
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

set_target_properties(job_wrapper_utest
    PROPERTIES
    OUTPUT_NAME job_wrapper_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET job_wrapper_utest
    TASK_NUM 1
)

