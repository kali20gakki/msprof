protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)
set(commonSrcFiles
    ${MSPROF_SOURCE_DIR}//dvvp/adx/wrapper/adx_prof_api.cpp    
    ${MSPROF_SOURCE_DIR}//dvvp/common/utils/utils.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/config/config_manager.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/thread/thread.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/thread/thread_pool.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/memory/chunk_pool.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/platform/platform.cpp    
    ${MSPROF_SOURCE_DIR}//dvvp/common/statistics/perf_count.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/app/application.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/message/codec.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/message/dispatcher.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/transport.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/file_transport.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/driver/devmgmt/ai_drv_dev_api.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/driver/devmgmt/mini/ai_drv_dsmi_api.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/driver/channel/ai_drv_prof_api.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/file_slice.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/file_ageing.cpp 
    ${CppPath}
)

set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}//dvvp/task_handle/src/prof_manager.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/task_handle/src/prof_task.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/task_handle/src/device.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/task_handle/src/info_json.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/task_handle/src/prof_params_adapter.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/task_handle/src/task_relationship_mgr.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/prof_channel_manager.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/job_factory.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/job_device_rpc.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/job_device_soc.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/collection_register.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/prof_timer.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/prof_job.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/job_wrapper/src/prof_peripheral_job.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/prof_channel.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/uploader.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/uploader_mgr.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/hdc/device_transport.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/hdc/hdc_transport.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/transport/hdc/platform/rpc/dev_mgr_api.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/validation/param_validation.cpp
    ${commonSrcFiles}
)

set(localSrcFiles
    test/main.cpp 
    ../stub/hdc-api-stub.cpp 
    ../stub/hdc-log-stub.cpp 
    ../stub/collect_io_server_stub.cpp 
    ../stub/common-utils-stub.cpp 
    ../stub/devdrv_runtime_api_stub.cpp 
    ../stub/profile_drv_user_stub.cpp 
    test/data_handle_utest.cpp    
    test/prof_host_core_utest.cpp 
    test/prof_manager_utest.cpp 
    test/prof_task_utest.cpp 
    test/prof_write_info_utest.cpp 
    test/prof_device_utest.cpp 
    test/prof_inotify_utest.cpp 
    test/prof_params_adapter_utest.cpp 
)

add_executable(host_utest
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(host_utest PRIVATE
    ${TOP_DIR}/abl/libc_sec/include 
    ${TOP_DIR}/inc/driver 
    ${TOP_DIR}/inc/tsch 
    ${TOP_DIR}/abl/adump 
    ${TOP_DIR}/abl/adump/external 
    ${TOP_DIR}/open_source/json/include 
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/metadef/inc
    ../stub/ 
    ${TOP_DIR}/llt/abl/msprof/msprof_cpp/st/stub 
    ${MSPROF_SOURCE_DIR}/dvvp 
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc    
    ${MSPROF_SOURCE_DIR}/dvvp/aging 
    ${MSPROF_SOURCE_DIR}/dvvp/common 
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
    ${MSPROF_SOURCE_DIR}/dvvp/common/config 
    # ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/ 
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation 
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics 
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/dvvp/host/include 
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/server/include 
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include 
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc 
    ${MSPROF_SOURCE_DIR}/dvvp/transport 
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include 
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(host_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(host_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(host_utest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    ascend_protobuf
    c_sec_static
)

set_target_properties(host_utest
    PROPERTIES
    OUTPUT_NAME host_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET host_utest
    TASK_NUM 1
)

