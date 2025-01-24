aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/json_tool DEVICE_TASK_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dump_tools/test DEVICE_TASK_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dump_tools/sample DEVICE_TASK_SOURCE)

add_executable(dump_tools_utest
    ${DEVICE_TASK_SOURCE}
)

target_include_directories(dump_tools_utest PRIVATE
    ${TOP_DIR}/analysis/csrc
    ${TOP_DIR}/opensource/rapidjson/include
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/dump_tools/
)

run_test(dump_tools_utest)
