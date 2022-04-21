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
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/driver_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/mmpa_plugin.cpp
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
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/opensource/securec/include
    ${TOP_DIR}/opensource/json/include
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/adump
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/mmpa
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/../inc
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/graphengine/external
    ${MSPROF_SOURCE_DIR}/../inc/toolchain
    ../stub/
    ${MSPROF_SOURCE_DIR}/dvvp 
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc    
    ${MSPROF_SOURCE_DIR}/dvvp/common 
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics 
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils 
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger 
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${MSPROF_SOURCE_DIR}/dvvp/common/queue
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
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
    -D_GLIBCXX_USE_CXX11_ABI=0
)

target_compile_definitions(transport PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(transport PRIVATE
    ascend_protobuf_static
    c_sec
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
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

