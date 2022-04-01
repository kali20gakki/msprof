file(GLOB_RECURSE SECUREC_Src ${SECUREC_DIR}/src/*.c)

set(SECUREC_Inc ${SECUREC_DIR}/include)

add_library(securec_share SHARED
    ${SECUREC_Src}
)

target_include_directories(securec_share PRIVATE
    ${SECUREC_Inc}
)

target_compile_options(securec_share PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

set_target_properties(securec_share
    PROPERTIES
    OUTPUT_NAME securec
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/securec
)

target_link_options(securec_share PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)