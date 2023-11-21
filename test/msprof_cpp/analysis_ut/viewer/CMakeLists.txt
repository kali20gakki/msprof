aux_source_directory(${TOP_DIR}/analysis/csrc/utils SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/viewer/database SOURCES)
aux_source_directory(${TOP_DIR}/analysis/csrc/dfx SOURCES)

add_executable(
    viewer_utest
    test/main.cpp
    test/connection_utest.cpp
    test/db_runner_utest.cpp
    ${SOURCES}
)

find_library(SQLITEPATH libsqlite3.so)
target_link_libraries(viewer_utest PRIVATE
    sqlite3 ${SQLITEPATH}
)

target_include_directories(viewer_utest PRIVATE
    ${GTEST_INCLUDE_DIR}
    ${MOCKCPP_INCLUDE_DIR}
    ${TOP_DIR}/analysis/csrc/utils
    ${TOP_DIR}/analysis/csrc/viewer/database
    ${TOP_DIR}/analysis/csrc/dfx
    ${TOP_DIR}/opensource/json/include
)

run_test(viewer_utest)
