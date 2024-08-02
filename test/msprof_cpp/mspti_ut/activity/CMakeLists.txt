file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

add_executable(activity_utest 
    ${SRC_FILE}
    test/main.cpp
    test/activity_utest.cpp
)

run_llt_test(
    TARGET activity_utest
    TASK_NUM 1
)
