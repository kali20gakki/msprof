aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dfx/ SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dfx/test SOURCES)

add_executable(dfx_utest
    ${SOURCES}
)

run_test(dfx_utest)
