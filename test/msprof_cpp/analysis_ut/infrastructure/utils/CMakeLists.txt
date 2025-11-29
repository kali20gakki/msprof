aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/tree SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dfx SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/utils/test SOURCES)

add_executable(utils_utest
    ${SOURCES}
)

run_test(utils_utest)
