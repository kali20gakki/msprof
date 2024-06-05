add_executable(activity_utest 
    ${TOP_DIR}/mspti/activity/activity_manager.cpp
    ${TOP_DIR}/mspti/common/function_loader.cpp
    ${TOP_DIR}/mspti/common/inject/plog_inject.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    test/main.cpp
    test/activity_utest.cpp
)

run_llt_test(
    TARGET activity_utest
    TASK_NUM 1
)
