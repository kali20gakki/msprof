aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/association/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/association/credential SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/finals SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/drafts SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/drafts/reflector SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/entities SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/host/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/environment SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/adapter SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/viewer/test SOURCES)

add_executable(viewer_utest
    ${SOURCES}
)

target_link_libraries(viewer_utest PRIVATE
    -lsqlite3
)

run_test(viewer_utest)
