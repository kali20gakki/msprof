add_executable(mspti_uti_utest 
    main.cpp
    utils_utest.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
)

target_include_directories(mspti_uti_utest PRIVATE
    ${UT_TOP_DIR}/ut_utils/
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
