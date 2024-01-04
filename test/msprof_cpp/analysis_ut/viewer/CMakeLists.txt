aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
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

add_executable(
    viewer_utest
    test/main.cpp
    test/connection_utest.cpp
    test/db_runner_utest.cpp
    test/cann_db_dumper_utest.cpp
    test/hash_db_dumper_utest.cpp
    test/type_info_dumper_utest.cpp
    ${TOP_DIR}/analysis/csrc/association/cann/tree_analyzer.cpp
    ${SOURCES}
)

target_link_libraries(viewer_utest PRIVATE
    -lsqlite3
)

include_directories(${TOP_DIR}/opensource/json/include)
include_directories(${GTEST_INCLUDE_DIR})
include_directories(${MOCKCPP_INCLUDE_DIR})
include_directories(${TOP_DIR}/analysis/csrc/association/credential)
include_directories(${TOP_DIR}/analysis/csrc/entities)
include_directories(${TOP_DIR}/analysis/csrc/viewer)
include_directories(${TOP_DIR}/analysis/csrc/viewer/database)
include_directories(${TOP_DIR}/analysis/csrc/viewer/database/finals)
include_directories(${TOP_DIR}/analysis/csrc/viewer/database/drafts)
include_directories(${TOP_DIR}/analysis/csrc/viewer/database/drafts/reflector)
include_directories(${TOP_DIR}/analysis/csrc/association/cann)
include_directories(${TOP_DIR}/analysis/csrc/utils)
include_directories(${TOP_DIR}/analysis/csrc/dfx)
include_directories(${TOP_DIR}/analysis/csrc/parser/host/cann)
include_directories(${TOP_DIR}/collector/inc/toolchain)
include_directories(${TOP_DIR}/analysis/csrc/parser)
include_directories(${TOP_DIR}/analysis/csrc/parser/adapter)
include_directories(${TOP_DIR}/analysis/csrc/parser/environment)

run_test(viewer_utest)
