aux_source_directory(${TOP_DIR}/analysis/csrc/utils DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/hal DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/metric DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/process DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/constant DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/metric DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/ DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/hccl DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/services/association/calculator/hccl/include
        DEVICE_ASSOCIATION_SOURCE)

aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/stubs/dfx DEVICE_ASSOCIATION_SOURCE)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/association/test DEVICE_ASSOCIATION_SOURCE)

add_executable(device_association_utest
    ${DEVICE_ASSOCIATION_SOURCE}
)

target_link_libraries(device_association_utest PRIVATE
    c_sec_static
)

run_test(device_association_utest)
