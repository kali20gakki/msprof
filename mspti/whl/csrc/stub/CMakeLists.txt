add_library(mspti_stub SHARED
    ${MSPTI_WHL_CSRC_DIR}/stub/mspti_stub.cpp
)

set_target_properties(mspti_stub
    PROPERTIES
    OUTPUT_NAME mspti
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/mspti_wrapper/stub
)

target_include_directories(mspti_stub PRIVATE
    ${TOP_DIR}/mspti
)

target_compile_options(mspti_stub PRIVATE
    -std=c++14
    -fPIC
    -fstack-protector-all
    -fno-strict-aliasing
    -fno-common
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wfloat-equal
    -Wextra
    -DMSPTI_LIB
    -D_FORTIFY_SOURCE=2
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-O2>
)

target_link_options(mspti_stub PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-s>
)

target_link_libraries(mspti_stub PRIVATE
    -Wl,--no-as-needed
    -Wl,--as-needed
)


install(TARGETS mspti_stub OPTIONAL
    LIBRARY DESTINATION mspti_wrapper/stub
)
