include_directories(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/common_stub/)
include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/)
include_directories(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/modeling/)
include_directories(${GTEST_INCLUDE_DIR})

aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_MODELING_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_MODELING_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_PARSER_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling DEVICE_MODELING_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/modeling/batch_id DEVICE_MODELING_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils DEVICE_MODELING_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_MODELING_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/modeling/test DEVICE_MODELING_SOURCE)

add_executable(
        device_modeling_utest
        ${DEVICE_MODELING_SOURCE}
)

run_test(device_modeling_utest)