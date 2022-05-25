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
    ../../stub/driver_plugin_stub.cpp
)

add_executable(transport_utest
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(transport_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/opensource/json/include
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${TOP_DIR}/collector/inc/
    ${TOP_DIR}/collector/inc/toolchain
    ${TOP_DIR}/collector/inc/external
    ${TOP_DIR}/collector/inc/external/acl
    ${TOP_DIR}/collector/inc/external/ge
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/adump
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/mmpa
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
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
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/
    # ${MSPROF_SOURCE_DIR}/dvvp/common/logger 
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

target_compile_options(transport_utest PRIVATE
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

target_compile_definitions(transport_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_options(transport_utest PRIVATE
	-lgcov
	--coverage
)

target_link_libraries(transport_utest PRIVATE
    ascend_protobuf_static
    c_sec_static
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

set_target_properties(transport_utest
    PROPERTIES
    OUTPUT_NAME transport_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET transport_utest
    TASK_NUM 1
)

