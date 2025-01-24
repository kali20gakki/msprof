aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/tree PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/adapter PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/environment PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/parser/host/cann PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dfx PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/host_parser/test PARSER_SOURCES)

add_executable(host_parser_utest
    ${PARSER_SOURCES}
)

run_test(host_parser_utest)
