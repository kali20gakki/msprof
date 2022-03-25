protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)

set(profilerSrcList
    ${MSPROF_SOURCE_DIR}//dvvp/common/validation/param_validation.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/app/application.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/app/env_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${CppPath}
)

set(profilerSrcFile
    test/main.cpp
    ../stub/hdc-api-stub.cpp
    ../stub/hdc-log-stub.cpp
    ../stub/common-utils-stub.cpp
    ../stub/devdrv_runtime_api_stub.cpp
    ../stub/profile_drv_user_stub.cpp
    test/application_utest.cpp
    ${profilerSrcList}
)


add_executable(
    application_utest
    ${profilerSrcFile}
)

target_include_directories(application_utest PRIVATE
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/inc/tsch
    ${TOP_DIR}/abl/adump
    ${TOP_DIR}/abl/adump/external
    ${TOP_DIR}/open_source/json/include
    ${TOP_DIR}/ace/npuruntime/inc
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/metadef/inc
    ../stub/
    ../stub/common
    ${MSPROF_SOURCE_DIR}/dvvp
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${MSPROF_SOURCE_DIR}/dvvp/aging
    ${MSPROF_SOURCE_DIR}/dvvp/common
    ${MSPROF_SOURCE_DIR}/dvvp/app
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/common/validation
    ${MSPROF_SOURCE_DIR}/dvvp/common/statistics
    ${MSPROF_SOURCE_DIR}/dvvp/host/include
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/server/include
    ${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/include
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
    ${MSPROF_SOURCE_DIR}/dvvp/transport
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include
    ${MSPROF_SOURCE_DIR}/dvvp/streamio/common/include
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
    ${CMAKE_BINARY_DIR}/proto/profiler/
)


target_compile_options(application_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(application_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(application_utest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<BUILD_INTERFACE:adump_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(application_utest
    PROPERTIES
    OUTPUT_NAME application_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET application_utest
    TASK_NUM 1
)
