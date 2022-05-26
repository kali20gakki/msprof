protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)
set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}/dvvp/common/memory/chunk_pool.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/common/src/select_operation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_slice.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/transport/file_ageing.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics/perf_count.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation/param_validation.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/thread/thread.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${CppPath}
)

set(localSrcFiles
	test/main.cpp 
	test/select_operation_test.cpp 
	test/file_slice_test.cpp 
	test/file_ageing_test.cpp
	../../stub/common-utils-stub.cpp 
	../../stub/hdc-log-stub.cpp
    ../../stub/devdrv_runtime_api_stub.cpp
    ../../../stub/driver_plugin_stub.cpp
)

add_executable(streamio_common_utest
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(streamio_common_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/opensource/json/include
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/adump
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/mmpa
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/../inc
	${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include 
	${MSPROF_SOURCE_DIR}/dvvp/ 
	${MSPROF_SOURCE_DIR}/dvvp/common 
	${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/
	# ${MSPROF_SOURCE_DIR}/dvvp/common/logger
	${MSPROF_SOURCE_DIR}/dvvp/common/validation 
	${MSPROF_SOURCE_DIR}/dvvp/aging 
	${MSPROF_SOURCE_DIR}/dvvp/transport 
	${MSPROF_SOURCE_DIR}/dvvp/common/statistics
	${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${TOP_DIR}/abl/libc_sec/include 
    ${TOP_DIR}/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(streamio_common_utest PRIVATE
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

target_compile_definitions(streamio_common_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_options(streamio_common_utest PRIVATE
	-lgcov
	--coverage
)

target_link_libraries(streamio_common_utest PRIVATE
    ascend_protobuf_static
    c_sec_static
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

set_target_properties(streamio_common_utest
    PROPERTIES
    OUTPUT_NAME streamio_common_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET streamio_common_utest
    TASK_NUM 1
)

