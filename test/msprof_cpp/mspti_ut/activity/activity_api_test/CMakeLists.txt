file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

add_executable(activity_utest 
    ${SRC_FILE}
    main.cpp
    activity_utest.cpp
)

set_target_properties(activity_utest
    PROPERTIES
    OUTPUT_NAME activity_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET activity_utest
    TASK_NUM 1
)
