project (test_nneng)

if (ENABLE_LLT_COV)
    set(COVERAGE_COMPILER_FLAGS "-g --coverage -fprofile-arcs -fPIC -O0 -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${COVERAGE_COMPILER_FLAGS}")
endif()



add_subdirectory(depends/runtime)
add_subdirectory(depends/te_fusion)
add_subdirectory(stub)
add_subdirectory(ut)
add_subdirectory(st)
