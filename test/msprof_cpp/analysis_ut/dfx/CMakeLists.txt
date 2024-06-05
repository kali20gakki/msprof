aux_source_directory(${TOP_DIR}/analysis/csrc/utils/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx/ SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/dfx/test SOURCES)

add_executable(dfx_utest
    ${SOURCES}
)

run_test(dfx_utest)
