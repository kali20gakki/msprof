add_executable(mspti_parser_utest 
    ${SRC_FILE}
    main.cpp
    parser_utest.cpp
    ${TOP_DIR}/mspti/activity/activity_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/parser_manager.cpp
    ${TOP_DIR}/mspti/common/context_manager.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    ${TOP_DIR}/mspti/common/thread_pool.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_task_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_prof_task.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_reader.cpp
)

target_include_directories(mspti_parser_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
)

target_link_libraries(mspti_parser_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
)

set_target_properties(mspti_parser_utest
    PROPERTIES
    OUTPUT_NAME mspti_parser_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET mspti_parser_utest
    TASK_NUM 1
)
