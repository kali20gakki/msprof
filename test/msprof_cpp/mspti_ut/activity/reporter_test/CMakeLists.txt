add_executable(mspti_reporter_utest 
    ${SRC_FILE}
    main.cpp
    memory_reporter_utest.cpp
    ${TOP_DIR}/mspti/activity/activity_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/reporter/memory_reporter.cpp
    ${TOP_DIR}/mspti/activity/ascend/reporter/external_correlation_reporter.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/parser_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/hccl_calculator.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/hccl_reporter.cpp
    ${TOP_DIR}/mspti/common/context_manager.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    ${TOP_DIR}/mspti/common/runtime_utils.cpp
    ${TOP_DIR}/mspti/common/thread_pool.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_task_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_prof_task.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_reader.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/hccl_inject_stub.cpp
)

target_include_directories(mspti_reporter_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
)

target_link_libraries(mspti_reporter_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
)

set_target_properties(mspti_reporter_utest
    PROPERTIES
    OUTPUT_NAME mspti_reporter_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET mspti_reporter_utest
    TASK_NUM 1
)
