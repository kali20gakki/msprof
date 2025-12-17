set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}//dvvp/message/prof_json_config.cpp
    ${MSPROF_SOURCE_DIR}//dvvp/common/utils/utils.cpp
    ${MSPROF_SOURCE_DIR}//dvvp/common/mmpa/mmpa_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_handle.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/driver_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/config/config_manager.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/platform/platform.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt/ai_drv_dev_api.cpp
    ${CppPath}
)

set(localSrcFiles
    test/main.cpp 
    test/message_test.cpp 
    test/prof_json_config_test.cpp
)

add_executable(message_utest 
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(message_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
	${TOP_DIR}/collector/dvvp/depend/inc/acl
    ${TOP_DIR}/collector/inc/toolchain
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/opensource/json/include
	${MSPROF_SOURCE_DIR}/dvvp/depend/inc/profapi
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/slog
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/../inc
    ${MSPROF_SOURCE_DIR}/dvvp
    ${MSPROF_SOURCE_DIR}/dvvp/common
    ${MSPROF_SOURCE_DIR}/dvvp/common/mmpa
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log/
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils 
    ${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/include
    ${MSPROF_SOURCE_DIR}/dvvp/depend/include/stub
    ${MSPROF_SOURCE_DIR}/dvvp/driver/inc
)

target_compile_options(message_utest PRIVATE
    -g
    -pipe
    -Wall
    -std=c++11
    -fno-access-control
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
    -fprofile-arcs
    -ftest-coverage
)

target_compile_definitions(message_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
    google=ascend_private
)

target_link_options(message_utest PRIVATE
	-lgcov
	--coverage
)

target_link_libraries(message_utest PRIVATE
    c_sec_static
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

set_target_properties(message_utest
    PROPERTIES
    OUTPUT_NAME message_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET message_utest
    TASK_NUM 1
)

