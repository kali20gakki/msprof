add_executable(
    utils_utest
    test/main.cpp
    test/safe_unordered_map_utest.cpp
    test/thread_pool_utest.cpp
    test/file_utest.cpp
    test/utils_utest.cpp
    ${TOP_DIR}/analysis/csrc/entities/event.cpp
    ${TOP_DIR}/analysis/csrc/utils/safe_unordered_map.h
    ${TOP_DIR}/analysis/csrc/utils/thread_pool.cpp
    ${TOP_DIR}/analysis/csrc/utils/file.cpp
    ${TOP_DIR}/analysis/csrc/utils/utils.cpp
    ${TOP_DIR}/analysis/csrc/dfx/log.cpp
)

target_include_directories(utils_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/analysis/csrc/utils
    ${TOP_DIR}/analysis/csrc/dfx
    ${TOP_DIR}/opensource/json/include
)

run_test(utils_utest)