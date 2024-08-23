file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

add_executable(context_manager_utest 
    ${SRC_FILE}
    main.cpp
    context_manager_utest.cpp
)

set_target_properties(context_manager_utest
    PROPERTIES
    OUTPUT_NAME context_manager_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET context_manager_utest
    TASK_NUM 1
)
