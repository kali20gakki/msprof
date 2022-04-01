project(fe)

file(GLOB_RECURSE PROTO_LIST RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "${METADEF_DIR}/proto/task.proto"
        "${METADEF_DIR}/proto/ge_ir.proto"
        )

protobuf_generate(fe_proto PROTO_SRCS PROTO_HDRS ${PROTO_LIST} TARGET)


file(GLOB FE_SRC_CPP_1 RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "adapter/common/*.cc"
        "adapter/dsa_adapter/*.cc"
        "adapter/tbe_adapter/*.cc"
        "adapter/tbe_adapter/tbe_info/*.cc"
        "common/*.cc"
        "common/format/*.cc"
        "common/fusion_statistic/*.cc"
        "common/graph/*.cc"
        "common/util/*.cc"
        "format_selector/*.cc"
        "format_selector/builtin/*.cc"
        "format_selector/builtin/broadcast/*.cc"
        "format_selector/builtin/broadcast/format_process/*.cc"
        "format_selector/builtin/process/*.cc"
        "format_selector/builtin/reduce/*.cc"
        "format_selector/builtin/reduce/reduce_format_selector/*.cc"
        "format_selector/common/*.cc"
        "format_selector/manager/*.cc"
        "format_selector/op_customize/*.cc"
        "format_selector/op_kernel/*.cc"
        "fusion_config_manager/*.cc"
        "fusion_manager/*.cc"
        "fusion_rule_manager/*.cc"
        "fusion_rule_manager/fusion_rule_data/*.cc"
        "fusion_rule_manager/fusion_rule_parser/*.cc"
        "graph_optimizer/*.cc"
        "graph_optimizer/ffts/*.cc"
        "graph_optimizer/dsa_optimizer/*.cc"
        "graph_optimizer/fusion_common/*.cc"
        "graph_optimizer/graph_fusion/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/checker/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/node_optimize/common/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/*.cc"
        "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/requant_fusion_pass/*.cc"
        "graph_optimizer/heavy_format_propagation/*.cc"
        "graph_optimizer/node_optimizer/*.cc"
        "graph_optimizer/op_axis_update/*.cc"
        "graph_optimizer/op_compiler/*.cc"
        "graph_optimizer/op_judge/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/strategy/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/strategy/dtype_strategy/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/strategy/format_strategy/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/dtype/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/strategy/matcher/format/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/update_desc/*.cc"
        "graph_optimizer/op_judge/format_and_dtype/update_desc/subgraph/*.cc"
        "graph_optimizer/op_judge/imply_type/*.cc"
        "graph_optimizer/op_setter/*.cc"
        "graph_optimizer/range_format_transfer/*.cc"
        "graph_optimizer/shape_format_transfer/*.cc"
        "graph_optimizer/shape_format_transfer/trans_node_implementation/*.cc"
        "graph_optimizer/shape_format_transfer/trans_node_manager/*.cc"
        "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_insertion/*.cc"
        "graph_optimizer/shape_format_transfer/trans_node_manager/trans_node_merging/*.cc"
        "graph_optimizer/spacesize_calculator/*.cc"
        "graph_optimizer/stream_graph_optimizer/*.cc"
        "graph_optimizer/stream_graph_optimizer/l2_optimizer/*.cc"
        "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_allocation/*.cc"
        "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_comm/*.cc"
        "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_handler/*.cc"
        "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_parser/*.cc"
        "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_fusion_rtl2ctrl/*.cc"
        "graph_optimizer/stream_graph_optimizer/l2_optimizer/l2_net_fusion/*.cc"
        "graph_optimizer/ub_fusion/*.cc"
        "graph_optimizer/ub_fusion/fusion_graph_merge/*.cc"
        "graph_optimizer/ub_fusion/tbe_pass/*.cc"
        "graph_optimizer/ub_fusion/tbe_pass/ub_pass_slice_info/*.cc"
        "graph_optimizer/fuzzy_compiler/*.cc"
        "itf_handler/*.cc"
        "ops_kernel_store/*.cc"
        "ops_kernel_store/op_kernel_constructor/*.cc"
        "cmo/*.cc")

# compile for fwkacllib
add_library(fe SHARED
        ${FE_SRC_CPP_1}
        ${PROTO_HDRS})

add_dependencies(fe fe_proto)

target_include_directories(fe PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${AIR_ROOT_INC_DIR}/framework
        ${METADEF_DIR}/inc/
        ${METADEF_DIR}/inc/common/opskernel/
        ${METADEF_DIR}/inc/common/optimizer/
        ${METADEF_DIR}/inc/common/util/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/external/graph/
        ${METADEF_DIR}/inc/external/framework/
        ${METADEF_DIR}/inc/graph
        ${METADEF_DIR}/inc/register/
        ${TOP_DIR}/graphengine/inc
        ${TOP_DIR}/graphengine/inc/external
        ${TOP_DIR}/graphengine/inc/framework
        ${CMAKE_BINARY_DIR}/proto/fe_proto)

target_compile_options(fe PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Wfloat-equal
        -Werror
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private
        -Wno-deprecated-declarations)

target_compile_definitions(fe PRIVATE
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(fe PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:tensor_engine_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:graph_tuner_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
        platform
        slog
        graph
        register
        c_sec
        runtime
        ascend_protobuf
        compress
        error_manager
        aicore_utils
        opskernel
        -O2
        -Wl,--no-as-needed
        json
        -ldl)

# compile for atc
add_library(atc_fe SHARED
        ${FE_SRC_CPP_1}
        ${PROTO_HDRS})

set_target_properties(atc_fe
        PROPERTIES
        OUTPUT_NAME fe
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/atclib)

add_dependencies(atc_fe fe_proto)

target_include_directories(atc_fe PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${AIR_ROOT_INC_DIR}/framework
        ${METADEF_DIR}/inc/
        ${METADEF_DIR}/inc/common/opskernel/
        ${METADEF_DIR}/inc/common/optimizer/
        ${METADEF_DIR}/inc/common/util/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/external/graph/
        ${METADEF_DIR}/inc/external/framework/
        ${METADEF_DIR}/inc/graph
        ${METADEF_DIR}/inc/register/
        ${TOP_DIR}/graphengine/inc
        ${TOP_DIR}/graphengine/inc/external
        ${TOP_DIR}/graphengine/inc/framework
        ${CMAKE_BINARY_DIR}/proto/fe_proto)

target_compile_definitions(atc_fe PRIVATE
    COMPILE_OMG_PACKAGE
    )

target_compile_definitions(atc_fe PRIVATE
    $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
    )

target_compile_options(atc_fe PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -Wfloat-equal
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private)

target_link_libraries(atc_fe PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:tensor_engine_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:graph_tuner_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
        platform
        slog
        graph
        register
        c_sec
        runtime
        ascend_protobuf
        compress
        error_manager
        atc_aicore_utils
        atc_opskernel
        -O2
        -Wl,--no-as-needed
        json
        -ldl)

add_custom_target(
        fe.ini ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fe.ini
)
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fe.ini
        COMMAND cp ${CMAKE_CURRENT_LIST_DIR}/fe_config/fe.ini ${CMAKE_CURRENT_BINARY_DIR}/
)

add_custom_target(
        fusion_config.json ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/fusion_config.json
)
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/fusion_config.json
        COMMAND cp ${CMAKE_CURRENT_LIST_DIR}/fe_config/fusion_config.json ${CMAKE_CURRENT_BINARY_DIR}/
)

set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS fe OPTIONAL
        EXPORT fe-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )

install(TARGETS atc_fe OPTIONAL
        EXPORT atc_fe-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}/atclib
        )

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/fe.ini
        ${CMAKE_CURRENT_BINARY_DIR}/fusion_config.json
        DESTINATION ${INSTALL_LIBRARY_DIR} OPTIONAL
        )
