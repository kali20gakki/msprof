include_directories(
    ${TOP_DIR}/analysis/csrc
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/entities/
)

add_subdirectory(modeling)
add_subdirectory(entities)
add_subdirectory(init)
add_subdirectory(association)
add_subdirectory(persistence)
add_subdirectory(data_process)
add_subdirectory(services)
add_subdirectory(host_parser)
add_subdirectory(worker)