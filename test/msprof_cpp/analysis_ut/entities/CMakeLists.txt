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
        ${TOP_DIR}/
        ${GTEST_INCLUDE_DIR}
        ${MOCKCPP_INCLUDE_DIR}
        ${TOP_DIR}/opensource/json/include
)

run_test(entities_utest)