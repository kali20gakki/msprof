file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

file(GLOB_RECURSE REMOVE_SOURCES
    ${TOP_DIR}/mspti/common/inject/*.cpp
)

list(REMOVE_ITEM SRC_FILE ${REMOVE_SOURCES})

add_executable(callback_utest
    ${SRC_FILE}
    main.cpp
    callback_utest.cpp
    ${TOP_DIR}/mspti/common/inject/mstx_inject.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/prof_common_stub.cpp
)

set_target_properties(callback_utest
    PROPERTIES
    OUTPUT_NAME callback_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET callback_utest
    TASK_NUM 1
)
