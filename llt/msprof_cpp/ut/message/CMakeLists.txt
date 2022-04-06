protobuf_generate(profiler
	CppPath
	HeaderPath
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
	${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)
set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}//dvvp/message/codec.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/message/dispatcher.cpp 
    ${MSPROF_SOURCE_DIR}//dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${CppPath}
)

set(localSrcFiles
    test/main.cpp 
    test/codec_test.cpp 
    test/dispatcher_test.cpp 
    test/message_test.cpp 
    ../stub/hdc-log-stub.cpp 
    ../stub/common-utils-stub.cpp 
)

add_executable(message_utest 
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(message_utest PRIVATE
    ${TOP_DIR}/abl/libc_sec/include 
    ${TOP_DIR}/inc/driver 
    ${TOP_DIR}/abl/adump 
    ${TOP_DIR}/open_source/json/include 
    ${TOP_DIR}/ace/npuruntime/inc  
    ${TOP_DIR}/abl/msprof/inc
    ${TOP_DIR}/metadef/inc
    ${MSPROF_SOURCE_DIR}/dvvp 
    ${MSPROF_SOURCE_DIR}/dvvp/common 
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger 
    ${CMAKE_BINARY_DIR}/proto/profiler/
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(message_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra

)

target_compile_definitions(message_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(message_utest PRIVATE
    $<BUILD_INTERFACE:intf_llt_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:slog_headers>
    ascend_protobuf
    c_sec
)

set_target_properties(message_utest
    PROPERTIES
    OUTPUT_NAME message_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET message_utest
    TASK_NUM 1
)

