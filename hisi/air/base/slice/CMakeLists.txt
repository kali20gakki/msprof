project (slice)

set(SRC_LIST
    "data_slice_helper.cc"
    "data_slice_factory.cc"
    "data_slice_default_impl.cc"
    "data_slice_elementwise_impl.cc")

#if (NOT ENABLE_D AND NOT ENABLE_ACL)
############ libslice.so ############
add_library(slice SHARED ${SRC_LIST})

target_compile_definitions(slice PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    HOST_VISIBILITY
    FMK_SUPPORT_DUMP
    OS_CENTOS
    google=ascend_private
    FUNC_VISIBILITY
)

target_compile_options(slice PRIVATE
    -fvisibility=default
    -O2
    -Werror
    -Wextra
    -Wfloat-equal
    -fno-common
)

target_include_directories(slice PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/base
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${CMAKE_BINARY_DIR}
    #### yellow zone ####
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    #### blue zone ####
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
)

target_link_options(slice PRIVATE
    -Wl,-Bsymbolic
)

target_link_libraries(slice PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    -Wl,--no-as-needed
    graph
    ascend_protobuf
    c_sec
    error_manager
    slog
    -Wl,--as-needed
    json
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS slice OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)
