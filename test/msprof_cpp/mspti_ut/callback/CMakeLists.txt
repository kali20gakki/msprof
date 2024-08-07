
add_executable(callback_utest 
    ${TOP_DIR}/mspti/callback/callback_manager.cpp
    ${TOP_DIR}/mspti/common/function_loader.cpp
    ${TOP_DIR}/mspti/common/inject/plog_inject.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    test/main.cpp
    test/callback_utest.cpp
)

run_llt_test(
    TARGET callback_utest
    TASK_NUM 1
)
