include_directories(${GTEST_INCLUDE_DIR})
include_directories(${MOCKCPP_INCLUDE_DIR})
include_directories(${TOP_DIR}/)
include_directories(${TOP_DIR}/opensource/json/include)

aux_source_directory(${TOP_DIR}/analysis/csrc/utils/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/entities/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx/ SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/utils/test SOURCES)

add_executable(
        utils_utest
        ${SOURCES}
)

run_test(utils_utest)