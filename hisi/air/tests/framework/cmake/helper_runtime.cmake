# ---- Test coverage ----

if (NOT DEFINED PRODUCT_SIDE)
    set(PRODUCT_SIDE "host")
endif ()

set(HELPER_RUNTIME_SRC
    ${AIR_CODE_DIR}/runtime/heterogeneous/common/config/configurations.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/common/config/device_debug_config.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/common/config/json_parser.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/common/data_flow/queue/helper_exchange_service.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/common/mem_grp/memory_group_manager.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/daemon/daemon_client_manager.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/daemon/daemon_service.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/daemon/deployer_daemon_client.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/helper_execution_runtime.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deploy_context.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deployer.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deployer_proxy.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/deployer_service_impl.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/helper_deploy_planner.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/deployer/master_model_deployer.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/datagw_manager.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/flow_route_manager.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/helper_exchange_deployer.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/flowrm/network_manager.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_client.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_data.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_manager.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_parser.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_service_impl.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/rank_parser.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/resource/device_info.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/resource/node_info.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/resource/resource_manager.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/executor/cpu_sched_event_dispatcher.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/executor/cpu_sched_model.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/executor/dynamic_model_executor.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/executor/event_handler.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/executor/executor_context.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/executor/incremental_model_parser.cc
    ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/communication_domain.cc
    ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/dgw_client_st_stub.cc
    ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/executor_manager_stub.cc
    ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/re_mem_group_st_stub.cc
#    ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/aicpusd_interface_stub.cc
)

set(GRPC_RELATED_SRC
        ${AIR_CODE_DIR}/runtime/heterogeneous/daemon/grpc_server.cc
        ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/rpc/deployer_client.cc
        ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_server.cc
        ${AIR_CODE_DIR}/runtime/heterogeneous/deploy/hcom/cluster/cluster_grpc_client.cc
        )

set(GRPC_STUB_SRC
        ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/deployer_client_stub.cc
        ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/cluster_grpc_client_stub.cc
        ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/cluster_server_stub.cc
        ${AIR_CODE_DIR}/tests/depends/helper_runtime/src/grpc_server_stub.cc
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
)

# ---- Target : helper runtime ----
set(GRPC_PROTO_LIST
    "${AIR_CODE_DIR}/runtime/heterogeneous/proto/deployer.proto"
    "${AIR_CODE_DIR}/runtime/heterogeneous/proto/cluster.proto"
)

protobuf_generate_grpc(deployer GRPC_PROTO_SRCS GRPC_PROTO_HDRS ${GRPC_PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")

add_library(helper_runtime SHARED
    ${HELPER_RUNTIME_SRC}
    ${GRPC_RELATED_SRC}
    ${GRPC_PROTO_SRCS}
    ${GRPC_PROTO_HDRS}
)

target_compile_definitions(helper_runtime PRIVATE
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
    google=ascend_private
)

target_include_directories(helper_runtime PUBLIC
    ${METADEF_DIR}
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${METADEF_DIR}/graph
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${CMAKE_BINARY_DIR}/proto_grpc/deployer
    ${AIR_CODE_DIR}
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/executor
    ${AIR_CODE_DIR}/runtime
    ${AIR_CODE_DIR}/runtime/heterogeneous
    ${CMAKE_BINARY_DIR}/proto
    ${METADEF_DIR}/third_party/fwkacllib/inc/mmpa
    ${AIR_CODE_DIR}/third_party/grpc/third_party/protobuf/src
    ${AIR_CODE_DIR}/third_party/inc
    ${CMAKE_BINARY_DIR}/opensrc/json/include
    ${CMAKE_BINARY_DIR}/grpc_build-prefix/src/grpc_build/include
    ${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build/include
    ${AIR_CODE_DIR}/third_party/inc/aicpu
    ${AIR_CODE_DIR}/third_party/inc/runtime
    ${CMAKE_BINARY_DIR}/proto/ge
    ${CMAKE_BINARY_DIR}/proto/ge/proto
)

target_compile_options(helper_runtime PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
    -Werror=format
)

target_link_libraries(helper_runtime PUBLIC
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    grpc++
    grpc
    -Wl,--no-as-needed
    ${STUB_LIBS}
    dl
    -Wl,--as-needed
    -lrt -ldl -lpthread -lgcov
)

set_target_properties(helper_runtime PROPERTIES CXX_STANDARD 11)
#add_dependencies(helper_runtime)


# ---- Target : helper runtime no_grpc ----
set(PROTO_LIST
        "${AIR_CODE_DIR}/runtime/proto/deployer.proto"
        "${AIR_CODE_DIR}/runtime/proto/cluster.proto"
        )

protobuf_generate(deployer PROTO_SRCS PROTO_HDRS ${GRPC_PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")

add_library(helper_runtime_no_grpc SHARED
        ${HELPER_RUNTIME_SRC}
        ${GRPC_STUB_SRC}
        ${PROTO_SRCS}
        ${PROTO_HDRS}
        )

target_compile_definitions(helper_runtime_no_grpc PRIVATE
        $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
        google=ascend_private
        )

target_include_directories(helper_runtime_no_grpc PUBLIC
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/graph
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/inc/external
        ${AIR_CODE_DIR}/inc/framework
        ${CMAKE_BINARY_DIR}/proto_grpc/deployer
        ${AIR_CODE_DIR}
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/executor
        ${AIR_CODE_DIR}/runtime
        ${AIR_CODE_DIR}/runtime/communication/cluster
        ${AIR_CODE_DIR}/runtime/communication/
        ${CMAKE_BINARY_DIR}/proto
        ${METADEF_DIR}/third_party/fwkacllib/inc/mmpa
        ${AIR_CODE_DIR}/third_party/grpc/third_party/protobuf/src
        ${AIR_CODE_DIR}/third_party/inc
        ${CMAKE_BINARY_DIR}/opensrc/json/include
        ${CMAKE_BINARY_DIR}/grpc_build-prefix/src/grpc_build/include
        ${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build/include
        ${AIR_CODE_DIR}/third_party/inc/aicpu
        ${AIR_CODE_DIR}/third_party/inc/runtime
        ${CMAKE_BINARY_DIR}/proto/ge
        ${CMAKE_BINARY_DIR}/proto/ge/proto
        )

target_compile_options(helper_runtime_no_grpc PRIVATE
        -g --coverage -fprofile-arcs -ftest-coverage
        -Werror=format
        )

target_link_libraries(helper_runtime_no_grpc PUBLIC
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        -Wl,--no-as-needed
        ${STUB_LIBS}
        dl
        -Wl,--as-needed
        -lrt -ldl -lpthread -lgcov
        )

set_target_properties(helper_runtime_no_grpc PROPERTIES CXX_STANDARD 11)
