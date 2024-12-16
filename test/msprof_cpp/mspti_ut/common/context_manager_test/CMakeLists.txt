add_executable(context_manager_utest 
    ${SRC_FILE}
    main.cpp
    context_manager_utest.cpp
    ${TOP_DIR}/mspti/common/context_manager.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
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
