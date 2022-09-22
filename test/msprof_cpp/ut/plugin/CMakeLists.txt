set(profilerSrcFiles
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/driver_plugin.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/src/plugin_handle.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/mmpa/mmpa_api.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/common/utils/utils.cpp
)

set(localSrcFiles
    test/main.cpp 
    test/driver_plugin_utest.cpp
    test/plugin_handle_utest.cpp
)

add_executable(plugin_utest 
    ${profilerSrcFiles}
    ${localSrcFiles}
)

target_include_directories(plugin_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/driver
    ${MSPROF_SOURCE_DIR}/dvvp/depend/inc/plugin
    ${MSPROF_SOURCE_DIR}/dvvp/common
    ${MSPROF_SOURCE_DIR}/dvvp/common/config
    ${MSPROF_SOURCE_DIR}/dvvp/common/mmpa
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/test/msprof_cpp/ut/stub/common/log
)

target_compile_options(plugin_utest PRIVATE
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

target_compile_definitions(plugin_utest PRIVATE
    C_SEC
    MSPROF_DEBUG
)

target_link_options(plugin_utest PRIVATE
	-lgcov
	--coverage
)

target_link_libraries(plugin_utest PRIVATE
    c_sec_static
    pthread
    ${GTEST_STATIC_LIBRARY}
    ${MOCKCPP_STATIC_LIBRARY}
    dl
)

set_target_properties(plugin_utest
    PROPERTIES
    OUTPUT_NAME plugin_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

run_llt_test(
    TARGET plugin_utest
    TASK_NUM 1
)