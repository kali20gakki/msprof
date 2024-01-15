include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/)
include_directories(${GTEST_INCLUDE_DIR})
include_directories(${MOCKCPP_INCLUDE_DIR})

aux_source_directory(${TOP_DIR}/analysis/csrc/parser PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/adapter PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/environment PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/host/cann PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/entities PARSER_SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/finals SOURCES)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/parser/test PARSER_SOURCES)
add_executable(
        parser_utest
        ${PARSER_SOURCES}
)

run_test(parser_utest)