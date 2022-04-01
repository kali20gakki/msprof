cmake_minimum_required(VERSION 3.14.1)
project(cpueng)

if (ENABLE_LLT_COV)
    set(COVERAGE_COMPILER_FLAGS "-g --coverage -fprofile-arcs -fPIC -O0 -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${COVERAGE_COMPILER_FLAGS}")
endif()

add_subdirectory(stub)

if (ENABLE_UT)
    add_subdirectory(ut)
    add_custom_target(aicpu_ut DEPENDS tf_engine_utest cpu_engine_utest host_engine_utest)
    add_custom_command(TARGET aicpu_ut
      COMMENT "=====run cpu test end====="
    )
endif()

if (ENABLE_ST)
    add_subdirectory(st)
    add_custom_target(aicpu_st DEPENDS tf_engine_stest cpu_engine_stest host_engine_stest)
    add_custom_command(TARGET aicpu_st
      COMMENT "=====run cpu test end====="
    )
endif()
