include_directories(
    ${TOP_DIR}/analysis/csrc
    ${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/entities/
)

add_subdirectory(modeling)
add_subdirectory(entities)
add_subdirectory(init)
add_subdirectory(services)
add_subdirectory(association)
add_subdirectory(persistence)
