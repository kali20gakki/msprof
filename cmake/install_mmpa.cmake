set(MMPA_Src
    ${MMPA_DIR}/src/mmpa_linux.c
)

set(MMPA_Inc 
    ${MMPA_DIR}/inc/mmpa
    ${MMPA_DIR}/inc/mmpa/sub_inc
    ${SECUREC_DIR}/include
)

add_library(mmpa_share SHARED
    ${MMPA_Src}
)

target_include_directories(mmpa_share PRIVATE
    ${MMPA_Inc}
)

target_compile_options(mmpa_share PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

target_compile_definitions(mmpa_share PRIVATE
    OS_TYPE=0
    FUNC_VISIBILITY
)

set_target_properties(mmpa_share
    PROPERTIES
    OUTPUT_NAME mmpa
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mmpa
)

target_link_options(mmpa_share PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)