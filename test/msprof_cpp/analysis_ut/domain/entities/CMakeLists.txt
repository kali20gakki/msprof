aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/metric DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/json_trace DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/json_tool DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/utils DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/tree DEVICE_TASK_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/entities/test DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_TASK_SOURCE)

add_executable(entities_utest
    ${DEVICE_TASK_SOURCE}
)

target_include_directories(entities_utest PRIVATE
    ${TOP_DIR}/opensource/rapidjson/include
)

run_test(entities_utest)
