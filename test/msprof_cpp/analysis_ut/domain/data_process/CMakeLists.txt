aux_source_directory(${TOP_DIR}/analysis/csrc/association/credential SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/data_inventory SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/infrastructure/db SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/ai_task SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/domain/data_process/system SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/environment SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/domain/data_process/test SOURCES)

add_executable(data_process_utest
        ${SOURCES}
)

target_link_libraries(data_process_utest PRIVATE
        -lsqlite3
)

run_test(data_process_utest)