include_directories(${TOP_DIR}/test/msprof_cpp/analysis_ut/infrastructure/common_stub/)
include_directories(${GTEST_INCLUDE_DIR})
include_directories(${MOCKCPP_INCLUDE_DIR})
include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/platform/securec/include)
include_directories(${TOP_DIR}/analysis/csrc)
include_directories(${TOP_DIR}/)

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

add_executable(
        device_association_utest
        ${DEVICE_ASSOCIATION_SOURCE}
)

target_link_libraries(device_association_utest PRIVATE
        c_sec_static
)

run_test(device_association_utest)
