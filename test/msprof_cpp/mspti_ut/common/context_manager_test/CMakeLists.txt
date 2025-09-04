add_executable(context_manager_utest 
    ${SRC_FILE}
    main.cpp
    context_manager_utest.cpp
    ${TOP_DIR}/mspti/common/inject/mstx_inject.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_task_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_prof_task.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_reader.cpp
    ${TOP_DIR}/mspti/activity/ascend/reporter/external_correlation_reporter.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/parser_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/cann_track_cache.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/communication_calculator.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/cann_hash_cache.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/device_task_calculator.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/mstx_parser.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/hccl_calculator.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/hccl_reporter.cpp
    ${TOP_DIR}/mspti/activity/activity_manager.cpp
    ${TOP_DIR}/mspti/common/context_manager.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    ${TOP_DIR}/mspti/common/function_loader.cpp
    ${TOP_DIR}/mspti/common/thread_pool.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/hccl_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/prof_common_stub.cpp
)

set_target_properties(context_manager_utest
    PROPERTIES
    OUTPUT_NAME context_manager_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(context_manager_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
)

target_link_libraries(context_manager_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
)

run_llt_test(
    TARGET context_manager_utest
    TASK_NUM 1
)
