aux_source_directory(${TOP_DIR}/analysis/csrc/entities/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils/ SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/entities/test SOURCES)

add_executable(
    entities_utest
    ${SOURCES}
)

run_test(entities_utest)
