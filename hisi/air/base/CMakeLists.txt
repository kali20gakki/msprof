project (ge_common)

add_subdirectory(slice)

set(SRC_LIST
    "common/auth/file_saver.cc"
    "common/bcast.cc"
    "common/context/ctx.cc"
    "common/cust_aicpu_kernel_store.cc"
    "common/debug/memory_dumper.cc"
    "common/dump/dump_manager.cc"
    "common/dump/dump_properties.cc"
    "common/fmk_error_codes.cc"
    "common/formats/format_transfers/datatype_transfer.cc"
    "common/formats/format_transfers/format_transfer_c1hwncoc0_hwcn.cc"
    "common/formats/format_transfers/format_transfer_dhwcn_fracz3D.cc"
    "common/formats/format_transfers/format_transfer_dhwnc_fracz3D_transpose.cc"
    "common/formats/format_transfers/format_transfer_fractal_nz.cc"
    "common/formats/format_transfers/format_transfer_fractal_z.cc"
    "common/formats/format_transfers/format_transfer_fractal_zz.cc"
    "common/formats/format_transfers/format_transfer_fractal_z_tbe.cc"
    "common/formats/format_transfers/format_transfer_fracz_hwcn.cc"
    "common/formats/format_transfers/format_transfer_fracz_nchw.cc"
    "common/formats/format_transfers/format_transfer_fracz_nhwc.cc"
    "common/formats/format_transfers/format_transfer_hwcn_c1hwncoc0.cc"
    "common/formats/format_transfers/format_transfer_nc1hwc0_nchw.cc"
    "common/formats/format_transfers/format_transfer_nc1hwc0_nhwc.cc"
    "common/formats/format_transfers/format_transfer_nchw_fz_c04.cc"
    "common/formats/format_transfers/format_transfer_nchw_nc1hwc0.cc"
    "common/formats/format_transfers/format_transfer_nhwc_nc1hwc0.cc"
    "common/formats/format_transfers/format_transfer_transpose.cc"
    "common/formats/formats.cc"
    "common/formats/register_format_transfer.cc"
    "common/formats/utils/formats_trans_utils.cc"
    "common/fp16_t.cc"
    "common/plugin/datatype_util.cc"
    "common/plugin/op_tiling_manager.cc"
    "common/plugin/plugin_manager.cc"
    "common/ge_format_util.cc"
    "common/ge_inner_error_codes.cc"
    "common/helper/model_helper.cc"
    "common/helper/om_file_helper.cc"
    "common/kernel_store.cc"
    "common/local_context.cc"
    "common/model/ge_model.cc"
    "common/model/ge_root_model.cc"
    "pne/model/flow_model.cc"
    "common/model/model_relation.cc"
    "common/model_parser/model_parser.cc"
    "common/model_saver.cc"
    "common/omg_util.cc"
    "common/op/attr_value_util.cc"
    "common/op/ge_op_utils.cc"
    "common/properties_manager.cc"
    "common/tbe_kernel_store.cc"
    "common/thread_pool.cc"
    "common/transop_util.cc"
    "common/types.cc"
    "common/util.cc"
    "common/file_constant_util.cc"
    "common/tbe_handle_store/tbe_handle_store.cc"
    "common/tbe_handle_store/bin_register_utils.cc"
    "graph/manager/graph_var_manager.cc"
    "graph/manager/graph_manager_utils.cc"
    "graph/build/memory/var_mem_assign_util.cc"
    "graph/bin_cache/node_bin_selector_factory.cc"
    "graph/bin_cache/node_compile_cache_module.cc"
    "graph/bin_cache/op_binary_cache.cc"
    "common/profiling/profiling_properties.cc"
    "exec_runtime/deploy/deploy_planner.cc"
    "exec_runtime/execution_runtime.cc"
    "common/profiling_definitions.cc"
    "common/ge_inner_attrs.cc"
    "pne/manager/process_node_engine_manager.cc")

if (NOT ENABLE_D AND NOT ENABLE_ACL)
############ libge_common.so ############
add_library(ge_common SHARED ${SRC_LIST})

add_dependencies(ge_common
    graphengine_protos
)

target_compile_definitions(ge_common PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    HOST_VISIBILITY
    FMK_SUPPORT_DUMP
    OS_CENTOS
    google=ascend_private
    FUNC_VISIBILITY
)

target_compile_options(ge_common PRIVATE
    -fvisibility=default
    -O2
    -Werror
    -Wextra
    -Wfloat-equal
    -fno-common
)

target_include_directories(ge_common PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/graphengine_protos
    #### yellow zone ####
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/ace/comop/inc>
    #### blue zone ####
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
)

target_link_options(ge_common PRIVATE
    -Wl,-Bsymbolic
)

target_link_libraries(ge_common PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:runtime_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    static_mmpa
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

############ libge_common.a ############
add_library(ge_common_static STATIC ${SRC_LIST})

add_dependencies(ge_common_static
    graphengine_protos
)

target_compile_definitions(ge_common_static PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    HOST_VISIBILITY
    FMK_SUPPORT_DUMP
    OS_CENTOS
    google=ascend_private
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,OS_TYPE=WIN,OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0 NOMINMAX>
    LOG_CPP
    FUNC_VISIBILITY
)

target_compile_options(ge_common_static PRIVATE
    $<$<OR:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-fvisibility=hidden -O2 -Werror -Wextra -Wfloat-equal -fno-common>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_include_directories(ge_common_static PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/graphengine_protos
    #### yellow zone ####
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/ace/comop/inc>
    #### blue zone ####
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
)

target_link_libraries(ge_common_static PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:runtime_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    ascend_protobuf_static
    json
    c_sec
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Android>>:-lrt>
    -ldl
)

else ()
############ libge_common.so w/static protobuf ############
add_library(ge_common SHARED ${SRC_LIST})

add_dependencies(ge_common
    graphengine_protos
)

target_compile_definitions(ge_common PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    HOST_VISIBILITY
    FMK_SUPPORT_DUMP
    OS_CENTOS
    google=ascend_private
    LOG_CPP
    FUNC_VISIBILITY
)

target_compile_options(ge_common PRIVATE
    -fvisibility=default
    -O2
    -Werror
    -Wextra
    -Wfloat-equal
    -fno-common
)

target_include_directories(ge_common PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/graphengine_protos
    #### yellow zone ####
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/ace/comop/inc>
    #### blue zone ####
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
)

target_link_options(ge_common PRIVATE
    -Wl,-Bsymbolic
)

target_link_libraries(ge_common PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:runtime_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    ascend_protobuf_static
    -Wl,--no-as-needed
    graph
    c_sec
    error_manager
    slog
    static_mmpa
    -Wl,--as-needed
    json
    -lrt
    -ldl
)
endif ()

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS ge_common OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)
