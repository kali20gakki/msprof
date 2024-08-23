file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

add_executable(mspti_uti_utest 
    ${SRC_FILE}
    main.cpp
    utils_utest.cpp
)

set_target_properties(mspti_uti_utest
    PROPERTIES
    OUTPUT_NAME mspti_uti_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET mspti_uti_utest
    TASK_NUM 1
)
