file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

file(GLOB_RECURSE REMOVE_SOURCES
    ${TOP_DIR}/mspti/common/inject/*.cpp
)

list(REMOVE_ITEM SRC_FILE ${REMOVE_SOURCES})

add_executable(activity_utest 
    ${SRC_FILE}
    main.cpp
    activity_utest.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
)

target_include_directories(activity_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
)

target_link_libraries(activity_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
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
