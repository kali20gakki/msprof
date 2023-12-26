add_executable(
        association_utest
        test/main.cpp
        test/tree_builder_utest.cpp
        test/id_pool_utest.cpp
        ${TOP_DIR}/analysis/csrc/association/cann/tree_builder.cpp
        ${TOP_DIR}/analysis/csrc/parser/host/cann/type_data.cpp
        ${TOP_DIR}/analysis/csrc/association/credential/id_pool.cpp
        ${TOP_DIR}/analysis/csrc/entities/event_queue.cpp
        ${TOP_DIR}/analysis/csrc/entities/event.cpp
        ${TOP_DIR}/analysis/csrc/entities/tree.cpp
        ${TOP_DIR}/analysis/csrc/utils/thread_pool.cpp
        ${TOP_DIR}/analysis/csrc/dfx/log.cpp
        ${TOP_DIR}/analysis/csrc/utils/file.cpp
        ${TOP_DIR}/analysis/csrc/utils/utils.cpp
        ${TOP_DIR}/analysis/csrc/parser/environment/context.cpp
)

target_include_directories(association_utest PRIVATE
        ${GTEST_INCLUDE_DIR}
        ${MOCKCPP_INCLUDE_DIR}
        ${TOP_DIR}/analysis/csrc/association/cann
        ${TOP_DIR}/analysis/csrc/parser/host/cann
        ${TOP_DIR}/analysis/csrc/parser/environment
        ${TOP_DIR}/analysis/csrc/association/credential
        ${TOP_DIR}/analysis/csrc/entities
        ${TOP_DIR}/analysis/csrc/parser/host/cann
        ${TOP_DIR}/analysis/csrc/utils
        ${TOP_DIR}/analysis/csrc/dfx
        ${TOP_DIR}/opensource/json/include
        ${TOP_DIR}/collector/inc/toolchain
        ${TOP_DIR}/test/msprof_cpp/analysis_ut/fake
)

run_test(association_utest)