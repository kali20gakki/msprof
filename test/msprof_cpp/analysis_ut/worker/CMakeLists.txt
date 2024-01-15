include_directories(${GTEST_INCLUDE_DIR})
include_directories(${MOCKCPP_INCLUDE_DIR})
include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${TOP_DIR}/collector/inc/toolchain)
include_directories(${TOP_DIR}/analysis/csrc/worker)
include_directories(${TOP_DIR}/analysis/csrc/viewer/database)
include_directories(${TOP_DIR}/analysis/csrc/viewer/database/drafts)
include_directories(${TOP_DIR}/analysis/csrc/parser/host/cann)
include_directories(${TOP_DIR}/analysis/csrc/dfx)
include_directories(${TOP_DIR}/analysis/csrc/utils)
include_directories(${TOP_DIR}/analysis/csrc/parser)
include_directories(${TOP_DIR}/analysis/csrc/entities)
include_directories(${TOP_DIR}/analysis/csrc/parser/adapter)
include_directories(${TOP_DIR}/analysis/csrc/parser/environment)
include_directories(${TOP_DIR}/analysis/csrc/association/cann)
include_directories(${TOP_DIR}/analysis/csrc/association/credential)

aux_source_directory(${TOP_DIR}/analysis/csrc/association/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/worker SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/host/cann SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database/drafts SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/adapter SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/entities SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/parser/environment SOURCES)
aux_source_directory(${TOP_DIR}/test/msprof_cpp/analysis_ut/worker/test SOURCES)

add_executable(
    worker_utest
    ${SOURCES}
)

target_link_libraries(worker_utest PRIVATE
    -lsqlite3
)

run_test(worker_utest)