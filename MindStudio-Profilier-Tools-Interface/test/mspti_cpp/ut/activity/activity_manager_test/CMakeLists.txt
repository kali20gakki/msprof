file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/csrc/activity/*.cpp
    ${TOP_DIR}/csrc/callback/*.cpp
    ${TOP_DIR}/csrc/common/*.cpp
)

file(GLOB_RECURSE REMOVE_SOURCES
    ${TOP_DIR}/csrc/common/inject/*.cpp
)

list(REMOVE_ITEM SRC_FILE ${REMOVE_SOURCES})

add_executable(activity_utest 
    ${SRC_FILE}
    main.cpp
    activity_utest.cpp
    ${TOP_DIR}/csrc/common/inject/mstx_inject.cpp
    ${TOP_DIR}/csrc/common/function_loader.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/acl_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/hccl_inject_stub.cpp
)

target_include_directories(activity_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/platform/securec/include
)

target_link_libraries(activity_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
    c_sec_static
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
