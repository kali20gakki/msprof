function(run_test test_case)
    target_compile_options(${test_case} PRIVATE
        -g
        -pipe
        -Wall
        -std=c++17
        -fno-access-control
        -Wfloat-equal
        -Wextra
        -D_GLIBCXX_USE_CXX11_ABI=0
        -fprofile-arcs
        -ftest-coverage
    )

    target_link_options(${test_case} PRIVATE
        -lgcov
        --coverage
    )

    target_link_libraries(${test_case} PRIVATE
        ${GTEST_STATIC_LIBRARY}
        ${MOCKCPP_STATIC_LIBRARY}
        pthread
        dl
    )

    set_target_properties(${test_case}
        PROPERTIES
        OUTPUT_NAME ${test_case}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    run_llt_test(
        TARGET ${test_case}
        TASK_NUM 1
    )
endfunction()
add_subdirectory(entities)
add_subdirectory(utils)