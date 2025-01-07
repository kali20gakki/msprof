aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/dump_tools/json_tool SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/environment SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/json_trace SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/ai_task SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/system SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/entities/viewer_data/ai_task SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/finals SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/timeline SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/summary SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/association/credential SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/application SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/application/assemble/test SOURCES)

add_executable(export_assemble_utest
        ${SOURCES}
)

target_link_libraries(export_assemble_utest PRIVATE
    -lsqlite3
)

run_test(export_assemble_utest)