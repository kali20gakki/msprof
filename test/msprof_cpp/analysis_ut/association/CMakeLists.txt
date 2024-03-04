include_directories(${GTEST_INCLUDE_DIR})
include_directories(${MOCKCPP_INCLUDE_DIR})
include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/)


aux_source_directory(${TOP_DIR}/analysis/csrc/association/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/host/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/adapter SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/association/credential SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/entities SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/environment SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/association/test SOURCES)

add_executable(
        association_utest
        ${SOURCES}
)

run_test(association_utest)