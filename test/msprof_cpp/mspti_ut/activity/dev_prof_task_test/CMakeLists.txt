file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

add_executable(dev_prof_task_utest 
    ${SRC_FILE}
    main.cpp
    dev_prof_task_utest.cpp
)

target_include_directories(dev_prof_task_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
)

target_link_libraries(dev_prof_task_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
)

set_target_properties(dev_prof_task_utest
    PROPERTIES
    OUTPUT_NAME dev_prof_task_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET dev_prof_task_utest
    TASK_NUM 1
)
