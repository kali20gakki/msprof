include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/analysis/csrc)
include_directories(${TOP_DIR}/)
include_directories(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/process_control/)
include_directories(${GTEST_INCLUDE_DIR})

aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils DEVICE_TASK_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/process_control/test DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/process_control/fake_process DEVICE_TASK_SOURCE)
add_executable(
        device_task_utest
        ${DEVICE_TASK_SOURCE}
)

run_test(device_task_utest)