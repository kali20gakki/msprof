include_directories(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/common_stub/)
include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/analysis/csrc)
include_directories(${TOP_DIR}/)
include_directories(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/init/)
include_directories(${GTEST_INCLUDE_DIR})

aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_INIT_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_INIT_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/init DEVICE_INIT_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_INIT_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils DEVICE_INIT_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/init/test DEVICE_INIT_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_INIT_SOURCE)

list(APPEND DEVICE_INIT_SOURCE ${TOP_DIR}/analysis/csrc/domain/services/device_context/cpu_info.cpp)
list(APPEND DEVICE_INIT_SOURCE ${TOP_DIR}/analysis/csrc/domain/services/device_context/device_context.cpp)
list(APPEND DEVICE_INIT_SOURCE ${TOP_DIR}/analysis/csrc/domain/services/device_context/device_info.cpp)
list(APPEND DEVICE_INIT_SOURCE ${TOP_DIR}/analysis/csrc/domain/services/device_context/load_start_log.cpp)
list(APPEND DEVICE_INIT_SOURCE ${TOP_DIR}/analysis/csrc/domain/services/device_context/sample_info.cpp)
add_executable(
        device_init_utest
        ${DEVICE_INIT_SOURCE}
)

run_test(device_init_utest)