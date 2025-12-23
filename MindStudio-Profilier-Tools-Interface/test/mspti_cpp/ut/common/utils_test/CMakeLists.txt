add_executable(mspti_utils_utest 
    main.cpp
    utils_utest.cpp
    ${TOP_DIR}/csrc/common/utils.cpp
    ${TOP_DIR}/csrc/common/runtime_utils.cpp
    ${TOP_DIR}/csrc/common/function_loader.cpp
    ${UT_TOP_DIR}/stub/inject/acl_inject_stub.cpp
)

target_include_directories(mspti_utils_utest PRIVATE
    ${UT_TOP_DIR}/ut_utils/
    ${MOCKCPP_INCLUDE_DIR}
)

target_link_libraries(mspti_utils_utest PRIVATE
        ${MOCKCPP_STATIC_LIBRARY}
)

set_target_properties(mspti_utils_utest
    PROPERTIES
    OUTPUT_NAME mspti_utils_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET mspti_utils_utest
    TASK_NUM 1
)
