include_directories(${GTEST_INCLUDE_DIR})
include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/)

aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling/batch_id DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils DEVICE_PARSER_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/services/test DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/services/stub DEVICE_PARSER_SOURCE)

list(REMOVE_ITEM DEVICE_PARSER_SOURCE ${TOP_DIR}/analysis/csrc/utils/file.cpp)
message(fatal ${DEVICE_PARSER_SOURCE})
add_executable(
        device_parser_utest
        ${DEVICE_PARSER_SOURCE}
)

run_test(device_parser_utest)
