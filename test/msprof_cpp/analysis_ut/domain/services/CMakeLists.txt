include_directories(${GTEST_INCLUDE_DIR})
include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/analysis/csrc)
include_directories(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/services/)
include_directories(${TOP_DIR}/)

aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling/batch_id DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/log DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/parser_item DEVICE_PARSER_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/services/test DEVICE_PARSER_SOURCE)

add_executable(
        device_parser_utest
        ${DEVICE_PARSER_SOURCE}
)

run_test(device_parser_utest)
