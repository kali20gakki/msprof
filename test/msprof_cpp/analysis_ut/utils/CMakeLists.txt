add_executable(
    xxx_utest
    test/main.cpp
    test/xxx_utest.cpp
    ${TOP_DIR}/analysis/csrc/xxx/xxx.cpp
)

target_include_directories(add_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/analysis/csrc/xxx
)

run_test(xxx_utest)