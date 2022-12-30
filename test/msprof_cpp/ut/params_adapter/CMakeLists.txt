protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

set(common_src_file
    ${CppPath}
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/mmpa/mmpa_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger/cmd_log.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/src/input_parser.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp
    ${TOP_DIR}/test/msprof_cpp/stub/driver_plugin_stub.cpp
)

set(src_file
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_msprof.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_aclapi.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_acljson.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_geopt.cpp
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
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/dynamic_profiling/src/dyn_prof_client.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp

)

set(include_path
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}/proto/profiler
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/
    
    ${MSPROF_SOURCE_DIR}/dvvp
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${MSPROF_SOURCE_DIR}/dvvp/common
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${MSPROF_SOURCE_DIR}/dvvp/common/mmpa
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/acl
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${MSPROF_SOURCE_DIR}/dvvp/message
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/dynamic_profiling/include
    ${MSPROF_SOURCE_DIR}/dvvp/msprofbin/include
    ${MSPROF_SOURCE_DIR}/dvvp/params_adapter
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include
    ${MSPROF_SOURCE_DIR}/dvvp/transport
    ${MSPROF_SOURCE_DIR}/inc/toolchain
    ${MSPROF_SOURCE_DIR}/inc/external/acl
)

add_executable(params_adapter_utest
    ${common_src_file}
    ${src_file}
    test/main.cpp
    test/params_adapter_utest.cpp
    test/params_adapter_aclapi_utest.cpp
    test/params_adapter_acljson_utest.cpp
    test/params_adapter_geopt_utest.cpp
    test/params_adapter_msprof_utest.cpp
)

target_include_directories(params_adapter_utest PRIVATE
    ${include_path}
)

target_compile_options(params_adapter_utest PRIVATE
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

target_compile_definitions(params_adapter_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(params_adapter_utest PRIVATE
    ascend_protobuf_static
    c_sec_static
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

target_link_options(params_adapter_utest PRIVATE
	-lgcov
	--coverage
)

set_target_properties(params_adapter_utest
    PROPERTIES
    OUTPUT_NAME params_adapter_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET params_adapter_utest
    TASK_NUM 1
)