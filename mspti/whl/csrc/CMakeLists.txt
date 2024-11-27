# Build libmspti.so stub
add_subdirectory(stub)

find_package(Python REQUIRED COMPONENTS Development)

add_library(mspti_wrapper SHARED
    ${MSPTI_WHL_CSRC_DIR}/init.cpp
    ${MSPTI_WHL_CSRC_DIR}/mspti_adapter.cpp
    ${CMAKE_SOURCE_DIR}/common/inject/plog_inject.cpp
    ${CMAKE_SOURCE_DIR}/common/function_loader.cpp
    ${CMAKE_SOURCE_DIR}/common/utils.cpp
)

set_target_properties(mspti_wrapper
    PROPERTIES
    OUTPUT_NAME mspti_C
)

target_include_directories(mspti_wrapper PRIVATE
    ${Python_INCLUDE_DIRS}
    ${TOP_DIR}/mspti
    ${TOP_DIR}/platform/securec/include
    ${MSPTI_WHL_CSRC_DIR}
)

target_compile_options(mspti_wrapper PRIVATE
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
    -DASCEND_CI_LIMITED_PY37 # 保持编译和运行python版本兼容性
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-O2>
)

target_link_directories(mspti_wrapper PRIVATE
    ${CMAKE_INSTALL_PREFIX}/mspti_wrapper/stub
    ${SECUREC_LIB_DIR}
)

target_link_options(mspti_wrapper PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-s>
)

target_link_libraries(mspti_wrapper PRIVATE
    -Wl,--no-as-needed
    -Wl,--as-needed
    mspti
    c_sec
)


install(TARGETS mspti_wrapper OPTIONAL
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/whl/mspti/lib64
)
