add_executable(
        entities_utest
        test/main.cpp
        test/event_queue_utest.cpp
        test/tree_utest.cpp
        ${TOP_DIR}/analysis/csrc/entities/event.cpp
        ${TOP_DIR}/analysis/csrc/entities/event_queue.cpp
        ${TOP_DIR}/analysis/csrc/entities/tree.cpp
        ${TOP_DIR}/analysis/csrc/dfx/log.cpp
        ${TOP_DIR}/analysis/csrc/utils/file.cpp
)

target_include_directories(entities_utest PRIVATE
        ${GTEST_INCLUDE_DIR}
        ${MOCKCPP_INCLUDE_DIR}
        ${TOP_DIR}/analysis/csrc/entities
        ${TOP_DIR}/analysis/csrc/utils
        ${TOP_DIR}/analysis/csrc/dfx
        ${TOP_DIR}/opensource/json/include
        ${TOP_DIR}/collector/inc/toolchain
)

run_test(entities_utest)