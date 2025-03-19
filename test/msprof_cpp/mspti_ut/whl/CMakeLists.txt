cmake_minimum_required (VERSION 3.16)

file(GLOB_RECURSE SRC_FILE
    ${TOP_DIR}/mspti/activity/*.cpp
    ${TOP_DIR}/mspti/callback/*.cpp
    ${TOP_DIR}/mspti/common/*.cpp
)

file(GLOB_RECURSE REMOVE_SOURCES
    ${TOP_DIR}/mspti/common/inject/*.cpp
)

list(REMOVE_ITEM SRC_FILE ${REMOVE_SOURCES})

find_package(Python REQUIRED COMPONENTS Development)

add_executable(mspti_adapter_utest 
    ${SRC_FILE}
    main.cpp
    mspti_adapter_utest.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    ${TOP_DIR}/mspti/whl/csrc/mspti_adapter.cpp
    ${UT_TOP_DIR}/stub/inject/driver_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/profapi_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/runtime_inject_stub.cpp
    ${UT_TOP_DIR}/stub/inject/hccl_inject_stub.cpp
)

target_include_directories(mspti_adapter_utest PRIVATE
    ${MOCKCPP_INCLUDE_DIR}
    ${Python_INCLUDE_DIRS}
)

target_link_libraries(mspti_adapter_utest PRIVATE
    ${MOCKCPP_STATIC_LIBRARY}
    ${Python_LIBRARIES}
)

set_target_properties(mspti_adapter_utest
    PROPERTIES
    OUTPUT_NAME mspti_adapter_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET mspti_adapter_utest
    TASK_NUM 1
)
