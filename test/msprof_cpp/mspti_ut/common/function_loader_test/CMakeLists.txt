add_library(profapi SHARED
    ./stub_so.cpp
)

set_target_properties(profapi
    PROPERTIES
    OUTPUT_NAME profapi
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib64
)

add_executable(function_loader_utest 
    main.cpp
    function_loader_utest.cpp
    ${TOP_DIR}/mspti/common/utils.cpp
    ${TOP_DIR}/mspti/common/function_loader.cpp
)

set_target_properties(function_loader_utest
    PROPERTIES
    OUTPUT_NAME function_loader_utest
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(function_loader_utest profapi)

run_llt_test(
    TARGET function_loader_utest
    TASK_NUM 1
)
