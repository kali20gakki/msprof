# ---- Test coverage ----

if (ENABLE_GE_COV)
    set(COVERAGE_COMPILER_FLAGS "-g --coverage -fprofile-arcs -fPIC -O0 -ftest-coverage")
    set(CMAKE_CXX_FLAGS "${COVERAGE_COMPILER_FLAGS}")
endif()

# ----metadef Proto generate ----
set(PROTO_LIST
    "${METADEF_DIR}/proto/om.proto"
    "${METADEF_DIR}/proto/ge_ir.proto"
    "${METADEF_DIR}/proto/insert_op.proto"
    "${METADEF_DIR}/proto/task.proto"
    "${METADEF_DIR}/proto/dump_task.proto"
    "${METADEF_DIR}/proto/fwk_adapter.proto"
    "${METADEF_DIR}/proto/op_mapping.proto"
    "${METADEF_DIR}/proto/ge_api.proto"
    "${METADEF_DIR}/proto/optimizer_priority.proto"
    "${METADEF_DIR}/proto/onnx/ge_onnx.proto"
    "${METADEF_DIR}/proto/tensorflow/attr_value.proto"
    "${METADEF_DIR}/proto/tensorflow/function.proto"
    "${METADEF_DIR}/proto/tensorflow/graph.proto"
    "${METADEF_DIR}/proto/tensorflow/graph_library.proto"
    "${METADEF_DIR}/proto/tensorflow/node_def.proto"
    "${METADEF_DIR}/proto/tensorflow/op_def.proto"
    "${METADEF_DIR}/proto/tensorflow/resource_handle.proto"
    "${METADEF_DIR}/proto/tensorflow/tensor.proto"
    "${METADEF_DIR}/proto/tensorflow/tensor_shape.proto"
    "${METADEF_DIR}/proto/tensorflow/types.proto"
    "${METADEF_DIR}/proto/tensorflow/versions.proto"
    "${METADEF_DIR}/proto/var_manager.proto"
)

protobuf_generate(ge PROTO_SRCS PROTO_HDRS ${PROTO_LIST} "--proto_path=${METADEF_DIR}/proto" TARGET)

# ---- File glob by group ----
file(GLOB_RECURSE METADEF_SRCS CONFIGURE_DEPENDS
    "${METADEF_DIR}/graph/*.cc"
    "${METADEF_DIR}/register/*.cc"
    "${METADEF_DIR}/register/op_tiling/*.cc"
    "${METADEF_DIR}/register/*.cpp"
    "${METADEF_DIR}/ops/*.cc"
    "${METADEF_DIR}/third_party/transformer/src/*.cc"
)

file(GLOB_RECURSE METADEF_REGISTER_SRCS CONFIGURE_DEPENDS
    "${METADEF_DIR}/register/*.cc"
    "${METADEF_DIR}/register/op_tiling/*.cc"
    "${METADEF_DIR}/register/*.cpp"
)

file(GLOB_RECURSE PARSER_SRCS CONFIGURE_DEPENDS
    "${PARSER_DIR}/parser/common/*.cc"
    "${PARSER_DIR}/parser/tensorflow/*.cc"
    "${PARSER_DIR}/parser/onnx/*.cc"
)

list(REMOVE_ITEM PARSER_SRCS
    "${PARSER_DIR}/parser/common/thread_pool.cc"
)

file(GLOB_RECURSE LOCAL_ENGINE_SRC CONFIGURE_DEPENDS
    "${AIR_CODE_DIR}/compiler/local_engine/*.cc"
)

list(REMOVE_ITEM LOCAL_ENGINE_SRC
    "${AIR_CODE_DIR}/compiler/local_engine/engine/host_cpu_engine.cc"
)

file(GLOB_RECURSE NN_ENGINE_SRC CONFIGURE_DEPENDS
    "${AIR_CODE_DIR}/compiler/plugin/*.cc"
)

file(GLOB_RECURSE OFFLINE_SRC CONFIGURE_DEPENDS
    "${AIR_CODE_DIR}/compiler/offline/*.cc"
)

file(GLOB_RECURSE GE_SRCS CONFIGURE_DEPENDS
    "${AIR_CODE_DIR}/base/common/*.cc"
    "${AIR_CODE_DIR}/base/slice/*.cc"
    "${AIR_CODE_DIR}/base/graph/*.cc"
    "${AIR_CODE_DIR}/base/pne/manager/*.cc"
    "${AIR_CODE_DIR}/base/pne/model/*.cc"
    "${AIR_CODE_DIR}/base/exec_runtime/*.cc"
    "${AIR_CODE_DIR}/compiler/analyzer/*.cc"
    "${AIR_CODE_DIR}/compiler/engine_manager/*.cc"
    "${AIR_CODE_DIR}/compiler/local_engine/*.cc"
    "${AIR_CODE_DIR}/compiler/opt_info/*.cc"
    "${AIR_CODE_DIR}/compiler/generator/*.cc"
    "${AIR_CODE_DIR}/compiler/graph/*.cc"
    "${AIR_CODE_DIR}/compiler/host_kernels/*.cc"
    "${AIR_CODE_DIR}/compiler/inc/*.cc"
    "${AIR_CODE_DIR}/compiler/init/*.cc"
    "${AIR_CODE_DIR}/compiler/ir_build/*.cc"
    "${AIR_CODE_DIR}/compiler/offline/*.cc"
    "${AIR_CODE_DIR}/compiler/opskernel_manager/*.cc"
    "${AIR_CODE_DIR}/compiler/plugin/*.cc"
    "${AIR_CODE_DIR}/compiler/pne/npu/*.cc"
    "${AIR_CODE_DIR}/compiler/pne/cpu/*.cc"
    "${AIR_CODE_DIR}/executor/*.cc"
    "${AIR_CODE_DIR}/runner/*.cc"
)

file(GLOB_RECURSE GE_SUB_ENGINE_SRCS CONFIGURE_DEPENDS
    "${AIR_CODE_DIR}/compiler/local_engine/engine/host_cpu_engine.cc"
)

list(REMOVE_ITEM GE_SRCS ${NN_ENGINE_SRC} "${AIR_CODE_DIR}/compiler/offline/main.cc")
list(APPEND GE_SRCS ${GE_SUB_ENGINE_SRCS})

add_library(graphengine_inc INTERFACE)

target_include_directories(graphengine_inc INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${AIR_CODE_DIR}/base"
    "${AIR_CODE_DIR}/compiler"
    "${AIR_CODE_DIR}/executor"
    "${AIR_CODE_DIR}/runner"
    "${AIR_CODE_DIR}/inc"
    "${METADEF_DIR}"
    "${METADEF_DIR}/graph"
    "${AIR_CODE_DIR}/inc/external"
    "${AIR_CODE_DIR}/inc/framework/common"
    "${METADEF_DIR}/inc/external"
    "${METADEF_DIR}/inc/external/graph"
    "${METADEF_DIR}/inc/graph"
    "${AIR_CODE_DIR}/inc/framework"
    "${METADEF_DIR}/inc/common"
    "${METADEF_DIR}/inc"
    "${METADEF_DIR}/register"
    "${METADEF_DIR}/third_party/transformer"
    "${METADEF_DIR}/third_party/transformer/inc"
    "${METADEF_DIR}/third_party/transformer"
    "${PARSER_DIR}"
    "${PARSER_DIR}/parser"
    "${PARSER_DIR}/inc"
    "${AIR_CODE_DIR}/third_party/inc"
    "${AIR_CODE_DIR}/third_party/inc/toolchain"
    "${AIR_CODE_DIR}/third_party/inc/opt_info"
    "${AIR_CODE_DIR}/tests/ut/ge"
    "${AIR_CODE_DIR}/tests/ut/common"
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_BINARY_DIR}/proto/ge"
    "${CMAKE_BINARY_DIR}/proto/ge/proto"
)

list(APPEND STUB_LIBS
    c_sec
    slog_stub
    runtime_stub
    mmpa_stub
    profiler_stub
    hccl_stub
    opt_feature_stub
    error_manager_stub
    ascend_protobuf
    json
    gflags
)

# ---- Target : metadef graph ----
add_library(metadef_graph STATIC
    ${METADEF_SRCS}
    ${PROTO_SRCS}
)

add_dependencies(metadef_graph ge)

target_compile_definitions(metadef_graph PRIVATE
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    google=ascend_private
    FMK_SUPPORT_DUMP
)

target_compile_options(metadef_graph PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(metadef_graph
    $<BUILD_INTERFACE:intf_pub>
    ${STUB_LIBS}
    graphengine_inc
    -lrt -ldl -lgcov
)

set_target_properties(metadef_graph PROPERTIES CXX_STANDARD 11)

# ---- Target : Local engine ----

add_library(local_engine SHARED
    ${LOCAL_ENGINE_SRC}
)

target_include_directories(local_engine PUBLIC
    "${AIR_CODE_DIR}/compiler/local_engine"
)

target_compile_definitions(local_engine PRIVATE
    google=ascend_private
)

target_compile_options(local_engine PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(local_engine PUBLIC
    $<BUILD_INTERFACE:intf_pub>
    ${STUB_LIBS}
    graphengine_inc
    -lrt -ldl -lpthread -lgcov
)

set_target_properties(local_engine PROPERTIES
    OUTPUT_NAME ge_local_engine
)

set_target_properties(local_engine PROPERTIES CXX_STANDARD 11)

# ---- Target : engine plugin----
#
add_library(nnengine SHARED
    ${NN_ENGINE_SRC}
)

target_include_directories(nnengine PUBLIC
    "${AIR_CODE_DIR}/compiler/plugin/engine"
)

target_compile_definitions(nnengine PRIVATE
    google=ascend_private
)

target_compile_options(nnengine PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(nnengine PUBLIC
    $<BUILD_INTERFACE:intf_pub>
    ${STUB_LIBS}
    graphengine_inc
    -lrt -ldl -lpthread -lgcov
)

set_target_properties(nnengine PROPERTIES CXX_STANDARD 11)

#   Targe: engine_conf
add_custom_target(
    engine_conf_json ALL
    DEPENDS ${CMAKE_BINARY_DIR}/engine_conf.json
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/engine_conf.json
    COMMAND cp ${AIR_CODE_DIR}/compiler/engine_manager/engine_conf.json ${CMAKE_BINARY_DIR}/
)

#   Targe: optimizer priority
add_custom_target(
    optimizer_priority_pbtxt ALL
    DEPENDS ${CMAKE_BINARY_DIR}/optimizer_priority.pbtxt
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/optimizer_priority.pbtxt
    COMMAND cp ${AIR_CODE_DIR}/compiler/opskernel_manager/optimizer_priority.pbtxt ${CMAKE_BINARY_DIR}/
)

# ---- Target : Graph engine ----
add_library(graphengine STATIC
    ${PARSER_SRCS}
    ${GE_SRCS}
)

target_include_directories(graphengine PUBLIC
    "${GE_CODE_DIR}/ge/host_cpu_engine"
)

target_compile_definitions(graphengine PRIVATE
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    google=ascend_private
    FMK_SUPPORT_DUMP
    FWK_SUPPORT_TRAINING_TRACE
)

target_compile_options(graphengine PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(graphengine PUBLIC
    $<BUILD_INTERFACE:intf_pub>
    graphengine_inc
    ${STUB_LIBS}
    -Wl,--whole-archive
    metadef_graph
    -Wl,--no-whole-archive
)

set_target_properties(graphengine PROPERTIES CXX_STANDARD 11)
add_dependencies(graphengine local_engine nnengine engine_conf_json optimizer_priority_pbtxt)
