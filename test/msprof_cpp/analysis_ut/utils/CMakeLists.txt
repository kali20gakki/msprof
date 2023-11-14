add_executable(
        utils_utest
        test/main.cpp
        test/safe_unordered_map_utest.cpp
        ${TOP_DIR}/analysis/csrc/entities/event.cpp
        ${TOP_DIR}/analysis/csrc/utils/safe_unordered_map.h
)

target_include_directories(utils_utest PRIVATE
        ${GTEST_INCLUDE_DIR}
        ${MOCKCPP_INCLUDE_DIR}
        ${TOP_DIR}/analysis/csrc/utils
)

run_test(utils_utest)