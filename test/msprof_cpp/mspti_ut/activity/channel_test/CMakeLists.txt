file(GLOB_RECURSE COMMOM_FILE
    ${TOP_DIR}/mspti/common/*.cpp
)

file(GLOB_RECURSE REMOVE_SOURCES
    ${TOP_DIR}/mspti/common/inject/*.cpp
)

list(REMOVE_ITEM COMMOM_FILE ${REMOVE_SOURCES})

set(SRC_FILE
    ${COMMOM_FILE}
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_reader.cpp
    ${TOP_DIR}/mspti/activity/ascend/parser/parser_manager.cpp
    ${TOP_DIR}/mspti/activity/activity_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_task_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/dev_prof_task.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool_manager.cpp
    ${TOP_DIR}/mspti/activity/ascend/channel/channel_pool.cpp
)

add_executable(mspti_channel_utest 
    ${SRC_FILE}
    main.cpp
    channel_utest.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
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
