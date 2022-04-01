protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)
set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}/dvvp/transport/transport.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/adx_prof_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/message/codec.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/uploader.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/uploader_mgr.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/transport/prof_channel.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread_pool.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/driver/channel/ai_drv_prof_api.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_slice.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_ageing.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hash_data.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/hdc_transport.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${CppPath}
)

set(localSrcFiles
    test/main.cpp 
    test/transport_utest.cpp 
    test/uploader_utest.cpp 
    test/prof_channel_utest.cpp
    test/hash_data_test.cpp
    ../stub/hdc-api-stub.cpp 
    ../stub/hdc-log-stub.cpp 
    ../stub/devdrv_runtime_api_stub.cpp 
    ../stub/profile_drv_user_stub.cpp 
    ../stub/common-utils-stub.cpp 
)

add_executable(transport
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(transport PRIVATE
    ${TOP_DIR}/abl/libc_sec/include 
    ${TOP_DIR}/inc/driver 
    ../stub/
    ${TOP_DIR}/llt/abl/msprof/msprof_cpp/st/stub 
    ${TOP_DIR}/abl/adump 
    ${TOP_DIR}/abl/adump/external 
    ${TOP_DIR}/open_source/json/include 
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/abl/msprof/inc/toolchain
    ${TOP_DIR}/metadef/inc
    ${MSPROF_SOURCE_DIR}/dvvp 
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc    
    ${MSPROF_SOURCE_DIR}/dvvp/common 
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics 
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils 
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger 
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${MSPROF_SOURCE_DIR}/dvvp/common/queue
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/common/include/
    ${MSPROF_SOURCE_DIR}/dvvp/message
    ${MSPROF_SOURCE_DIR}/dvvp/transport
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
    ${MSPROF_SOURCE_DIR}/dvvp/device/include 
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc 
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include 
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include 
    ${MSPROF_SOURCE_DIR}/dvvp/aging 
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(transport PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra

)

target_compile_definitions(transport PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(transport PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(transport
    PROPERTIES
    OUTPUT_NAME transport
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET transport
    TASK_NUM 1
)

