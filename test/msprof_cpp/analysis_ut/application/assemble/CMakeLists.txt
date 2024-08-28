aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/json_tool SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/environment SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/json_trace SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/viewer_data/ai_tsak SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/timeline SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/application/assemble/test SOURCES)

add_executable(timeline_assemble_utest
        ${SOURCES}
)

target_link_libraries(timeline_assemble_utest PRIVATE
    -lsqlite3
)

run_test(timeline_assemble_utest)