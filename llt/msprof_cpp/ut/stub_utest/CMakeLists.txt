
add_executable(stub_file_utest
    ${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/hdc_api.cpp  
    ${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/memory_utils.cpp
    test/main.cpp
    test/hdc_api_utest.cpp
    ..//stub/mmpa_stub.cpp
    ../stub/profile_drv_user_stub.cpp
    ../stub/hdc-log-stub.cpp
    ../stub/devdrv_runtime_api_stub.cpp
    ../stub/ide_hdc_stub.cc

)

target_include_directories(stub_file_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/opensource/securec/include
    ${TOP_DIR}/opensource/json/include
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/adump
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/mmpa
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/../inc
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/common/
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${MSPROF_SOURCE_DIR}/dvvp/adx/inc
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/wrapper
    ${MSPROF_SOURCE_DIR}/dvvp
)


target_compile_options(stub_file_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
)

target_compile_definitions(stub_file_utest PRIVATE
    C_SEC
    __PROF_UT
    google=ascend_private
)

target_link_libraries(stub_file_utest PRIVATE
    ascend_protobuf_static
    c_sec
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

set_target_properties(stub_file_utest
    PROPERTIES
    OUTPUT_NAME stub_file_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET stub_file_utest
    TASK_NUM 1
)
