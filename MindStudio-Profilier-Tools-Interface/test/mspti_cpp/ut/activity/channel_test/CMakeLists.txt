file(GLOB_RECURSE COMMON_FILE
    ${TOP_DIR}/csrc/common/*.cpp
)

file(GLOB_RECURSE REMOVE_SOURCES
    ${TOP_DIR}/csrc/common/inject/*.cpp
)

list(REMOVE_ITEM COMMON_FILE ${REMOVE_SOURCES})

set(SRC_FILE
    ${COMMON_FILE}
    ${TOP_DIR}/csrc/activity/ascend/channel/channel_reader.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/parser_manager.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/cann_hash_cache.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/device_task_calculator.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/communication_calculator.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/cann_track_cache.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/mstx_parser.cpp
    ${TOP_DIR}/csrc/activity/ascend/reporter/external_correlation_reporter.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/hccl_calculator.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/hccl_reporter.cpp
    ${TOP_DIR}/csrc/activity/activity_manager.cpp
    ${TOP_DIR}/csrc/activity/ascend/dev_task_manager.cpp
    ${TOP_DIR}/csrc/activity/ascend/dev_prof_task.cpp
    ${TOP_DIR}/csrc/activity/ascend/channel/channel_pool_manager.cpp
    ${TOP_DIR}/csrc/activity/ascend/channel/channel_pool.cpp
    ${TOP_DIR}/csrc/activity/ascend/parser/kernel_parser.cpp
    ${TOP_DIR}/csrc/activity/ascend/channel/soclog_convert.cpp
    ${TOP_DIR}/csrc/activity/ascend/channel/stars_common.cpp
)

add_executable(mspti_channel_utest 
    ${SRC_FILE}
    main.cpp
    channel_utest.cpp
    convert_utest.cpp
    ${TOP_DIR}/csrc/common/inject/mstx_inject.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/acl_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/hccl_inject_stub.cpp
)

target_include_directories(mspti_channel_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
)

target_link_libraries(mspti_channel_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
)

set_target_properties(mspti_channel_utest
    PROPERTIES
    OUTPUT_NAME mspti_channel_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET mspti_channel_utest
    TASK_NUM 1
)
