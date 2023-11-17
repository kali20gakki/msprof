add_executable(
    dfx_utest
    test/main.cpp
    test/log_utest.cpp
    ${TOP_DIR}/analysis/csrc/dfx/log.cpp
    ${TOP_DIR}/analysis/csrc/utils/file.cpp
)

target_include_directories(dfx_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/analysis/csrc/dfx
    ${TOP_DIR}/analysis/csrc/utils
    ${TOP_DIR}/opensource/json/include
)

run_test(dfx_utest)