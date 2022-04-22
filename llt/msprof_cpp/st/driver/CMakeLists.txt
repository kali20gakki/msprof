set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/driver/channel/ai_drv_prof_api.cpp 
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/mmpa_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/driver_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_manager.cpp
)

set(localSrcFiles
    testcase/main.cpp 
    ../stub/hdc-log-stub.cpp 
    ../stub/devdrv_runtime_api_stub.cpp 
    ../stub/profile_drv_user_stub.cpp 
    ../stub/common-utils-stub.cpp 
    testcase/ai_drv_dev_api_stest.cpp 
)

add_executable(driver_stest 
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(driver_stest PRIVATE
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
    ${MSPROF_SOURCE_DIR}/dvvp 
    ${MSPROF_SOURCE_DIR}/dvvp/task_handle/include 
    ${MSPROF_SOURCE_DIR}/dvvp/common 
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc 
    ${MSPROF_SOURCE_DIR}/dvvp/common/logger
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
)

target_compile_options(driver_stest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
)

target_compile_definitions(driver_stest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_libraries(driver_stest PRIVATE
    ascend_protobuf_static
    c_sec
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

set_target_properties(driver_stest
    PROPERTIES
    OUTPUT_NAME driver_stest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET driver_stest
    TASK_NUM 1
)
