aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling/batch_id DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/step_trace DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/environment DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/log DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/pmu DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/track DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/device_context DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/freq DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/parser_item DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling/step_trace DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/tree DEVICE_PARSER_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/services/test DEVICE_PARSER_SOURCE)

add_executable(device_parser_utest
    ${DEVICE_PARSER_SOURCE}
    ${TOP_DIR}/analysis/csrc/domain/services/persistence/host/number_mapping.cpp
)

target_include_directories(device_parser_utest PRIVATE
    ${TOP_DIR}/platform/securec/include
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/services/
)
target_link_libraries(device_parser_utest PRIVATE
        c_sec_static
        -lsqlite3
)

run_test(device_parser_utest)