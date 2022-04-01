cmake_minimum_required(VERSION 3.14)
project(heterogeneous_runtime)

if (NOT DEFINED PRODUCT_SIDE)
    set(PRODUCT_SIDE "host")
endif ()

set(GRPC_PROTO_LIST
        "${AIR_CODE_DIR}/runtime/heterogeneous/proto/deployer.proto"
        )

set(DEPLOYER_PROTO_LIST
        "${AIR_CODE_DIR}/runtime/heterogeneous/proto/deployer.proto"
        )
set(GRPC_SERVER_PROTO_LIST
        "${METADEF_DIR}/proto/ge_ir.proto"
        "${METADEF_DIR}/proto/var_manager.proto"
        "${METADEF_DIR}/proto/task.proto"
        "${AIR_CODE_DIR}/runtime/heterogeneous/proto/deployer.proto"
        )

protobuf_generate(deployer DEPLOYER_PROTO_SRCS DEPLOYER_PROTO_HDRS ${DEPLOYER_PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")
protobuf_generate_grpc(deployer GRPC_PROTO_SRCS GRPC_PROTO_HDRS  ${GRPC_PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")
protobuf_generate_grpc(deployer GRPC_SERVER_PROTO_SRCS GRPC_SERVER_PROTO_HDRS  ${GRPC_SERVER_PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")

set(CLUSTER_PROTO_LIST
        "${AIR_CODE_DIR}/runtime/heterogeneous/proto/cluster.proto"
)

protobuf_generate_grpc(cluster CLUSTER_PROTO_SRCS CLUSTER_PROTO_HDRS ${CLUSTER_PROTO_LIST} "--proto_path=${METADEF_DIR}/proto")


########################################## comm #################################
set(HELPER_COMMUNICATION_SRC_LIST
        deploy/hcom/cluster/cluster_manager.cc
        deploy/hcom/cluster/cluster_parser.cc
        deploy/hcom/cluster/cluster_server.cc
        deploy/hcom/cluster/cluster_data.cc
        deploy/hcom/cluster/cluster_client.cc
        deploy/hcom/cluster/cluster_grpc_client.cc
        deploy/hcom/cluster/cluster_service_impl.cc
        deploy/hcom/rank_parser.cc
        deploy/hcom/communication_domain.cc
        )

set(HELPER_COMMUNICATION_INC_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/communication
        ${CMAKE_BINARY_DIR}/proto_grpc/cluster
        )

########################################## client ###############################
set(HELPER_RUNTIME_SRC_LIST
        common/config/configurations.cc
        common/config/device_debug_config.cc
        common/config/json_parser.cc
        common/data_flow/queue/helper_exchange_service.cc
        common/mem_grp/memory_group_manager.cc
        daemon/deployer_daemon_client.cc
        deploy/deployer/deployer.cc
        deploy/deployer/deploy_context.cc
        deploy/deployer/deployer_proxy.cc
        deploy/deployer/deployer_service_impl.cc
        deploy/deployer/helper_deploy_planner.cc
        deploy/deployer/master_model_deployer.cc
        deploy/execfwk/executor_manager.cc
        deploy/helper_execution_runtime.cc
        deploy/flowrm/datagw_manager.cc
        deploy/flowrm/flow_route_manager.cc
        deploy/flowrm/helper_exchange_deployer.cc
        deploy/flowrm/network_manager.cc
        deploy/flowrm/queue_schedule_manager.cc
        deploy/resource/device_info.cc
        deploy/resource/node_info.cc
        deploy/resource/resource_manager.cc
        deploy/rpc/deployer_client.cc
        )

add_library(grpc_client SHARED
        ${HELPER_RUNTIME_SRC_LIST}
        ${GRPC_PROTO_HDRS}
        ${GRPC_PROTO_SRCS}
        ${HELPER_COMMUNICATION_SRC_LIST}
        ${CLUSTER_PROTO_HDRS}
        ${CLUSTER_PROTO_SRCS}
        )

include_directories(
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/graph
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/inc/external
        ${CMAKE_BINARY_DIR}/proto_grpc/deployer
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/heterogeneous
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/open_source/json/include>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc/aicpu>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/npuruntime/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl/msprof/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/air/inc/framework>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protobuf_build-prefix/src/protobuf_build/src/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build/include/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc/mmpa>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/inc/framework>
)

target_include_directories(grpc_client PRIVATE
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/graph
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/third_party/inc
        ${AIR_CODE_DIR}/inc/external
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos/proto
        ${CMAKE_BINARY_DIR}/proto_grpc/deployer
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/heterogeneous
        ${HELPER_COMMUNICATION_INC_LIST}
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/open_source/json/include>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc/aicpu>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/npuruntime/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl/msprof/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/air/inc/framework>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/metadef>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protobuf_build-prefix/src/protobuf_build/src/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build/include/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc/mmpa>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/metadef>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/inc/framework>
        )

target_compile_options(grpc_client PRIVATE
        -O2
        -fPIC
        -fno-common
        -Wextra
        -Wfloat-equal
        )

add_dependencies(grpc_client graphengine_protos)

target_compile_definitions(grpc_client PRIVATE
        google=ascend_private
        )

target_link_options(grpc_client PRIVATE
        -rdynamic
        -Wl,-Bsymbolic
        -Wl,--exclude-libs,ALL
        )

target_link_libraries(grpc_client PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        -Xlinker "-("
        grpc++
        grpc
        -Xlinker "-)"
        -Wl,--no-as-needed
        slog
        static_mmpa
        runtime
        c_sec
        ge_common
        error_manager
        dgw_client_so
        dl
        -Wl,--as-needed
        json
        )

########################################## model_deployer_daemon ###############################
set(DEPLOYER_DAEMON_SRC_FILE
        common/config/configurations.cc
        common/config/device_debug_config.cc
        common/config/json_parser.cc
        common/data_flow/queue/helper_exchange_service.cc
        common/mem_grp/memory_group_manager.cc
        daemon/daemon_client_manager.cc
        daemon/daemon_service.cc
        daemon/deployer_daemon_client.cc
        daemon/grpc_server.cc
        daemon/model_deployer_daemon.cc
        deploy/deployer/deploy_context.cc
        deploy/deployer/deployer_service_impl.cc
        deploy/execfwk/executor_manager.cc
        deploy/flowrm/datagw_manager.cc
        deploy/flowrm/flow_route_manager.cc
        deploy/flowrm/helper_exchange_deployer.cc
        deploy/flowrm/queue_schedule_manager.cc
        deploy/resource/device_info.cc
        )

set(DEPLOYER_DAEMON_INC_LIST
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/graph
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/inc/external
        ${AIR_CODE_DIR}/base
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/proto_grpc/deployer
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/open_source/json/include>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc/aicpu>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/npuruntime/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl/msprof/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/air/inc/framework>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc/mmpa>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/inc/framework>
        )

add_executable(grpc_server
        ${DEPLOYER_DAEMON_SRC_FILE}
        ${GRPC_SERVER_PROTO_HDRS}
        ${GRPC_SERVER_PROTO_SRCS}
        ${HELPER_COMMUNICATION_SRC_LIST}
        ${CLUSTER_PROTO_HDRS}
        ${CLUSTER_PROTO_SRCS}
        )

target_include_directories(grpc_server PRIVATE
        ${DEPLOYER_DAEMON_INC_LIST}
        ${HELPER_COMMUNICATION_INC_LIST}
        )

target_compile_options(grpc_server PRIVATE
        -O2
        -fPIC
        -fno-common
        -Wextra
        -Wfloat-equal
        )

target_compile_definitions(grpc_server PRIVATE
        #google=ascend_private
        )

target_link_options(grpc_server PRIVATE
        -rdynamic
        -Wl,-Bsymbolic
        -Wl,--exclude-libs,All
        $<$<STREQUAL:${PRODUCT},npuf10>:-Wl,-rpath-link,${TOP_DIR}/vendor/sdk/hi3796/drv>
        $<$<STREQUAL:${PRODUCT},npuf10>:-L$<TARGET_FILE_DIR:slog>>
        )

target_link_libraries(grpc_server PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        -Xlinker "-("
        grpc++
        grpc
        -Xlinker "-)"
        -Wl,--no-as-needed
        slog
        $<$<STREQUAL:${PRODUCT},npuf10>:-lslog>
        static_mmpa
        c_sec
        error_manager
        rt
        dl
        runtime
        graph
        ge_common
        dgw_client_so
        -Wl,--as-needed
        json
        )

########################################## npu_executor ###############################
set(NPU_EXECUTOR_INC_LIST
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/executor
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/third_party/inc
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos/proto
        ${CMAKE_BINARY_DIR}/proto/deployer
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/comop/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/comop/inc/external>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/npuruntime/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/air/inc/framework>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl/adump/external>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/inc/framework>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc/toolchain>
        )

set(NPU_EXECUTOR_SRC_LIST
        executor/cpu_sched_event_dispatcher.cc
        executor/cpu_sched_model.cc
        executor/dynamic_model_executor.cc
        executor/engine_daemon.cc
        executor/event_handler.cc
        executor/executor_context.cc
        executor/incremental_model_parser.cc
        executor/npu_executor.cc
        )

add_executable(npu_executor_main
        ${NPU_EXECUTOR_SRC_LIST}
        ${DEPLOYER_PROTO_HDRS}
        ${DEPLOYER_PROTO_SRCS}
        )

add_dependencies(npu_executor_main graphengine_protos)

target_include_directories(npu_executor_main PRIVATE
        ${NPU_EXECUTOR_INC_LIST}
        )

target_compile_options(npu_executor_main PRIVATE
        -g
        -fPIC
        -fno-common
        -Wextra
        -Wfloat-equal
        )

target_compile_definitions(npu_executor_main PRIVATE
        google=ascend_private)

target_link_options(npu_executor_main PRIVATE
        -rdynamic
        -Wl,-Bsymbolic
        -Wl,--exclude-libs,ALL)

target_link_libraries(npu_executor_main PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        -Wl,--no-as-needed
        ascend_protobuf
        slog
        alog
        error_manager
        c_sec
        runtime
        ascend_hal_stub
        msprofiler_fwk_share
        adump_server
        ge_common
        graph
        ge_executor_shared
        aicpu_scheduler_so_stub
        static_mmpa
        dl
        rt
        -Wl,--as-needed
        )


########################################## host_cpu_executor ###############################
set(HOST_CPU_EXECUTOR_INC_LIST
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/executor
        ${AIR_CODE_DIR}/inc
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos/proto
        ${CMAKE_BINARY_DIR}/proto/deployer
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/comop/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/comop/inc/external>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/npuruntime/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/air/inc/framework>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl/adump/external>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/inc/framework>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc/toolchain>
        )

set(HOST_CPU_EXECUTOR_SRC_LIST
        executor/cpu_sched_event_dispatcher.cc
        executor/cpu_sched_model.cc
        executor/dynamic_model_executor.cc
        executor/engine_daemon.cc
        executor/event_handler.cc
        executor/executor_context.cc
        executor/incremental_model_parser.cc
        executor/host_cpu_executor.cc
        )

add_executable(host_cpu_executor_main
        ${HOST_CPU_EXECUTOR_SRC_LIST}
        ${DEPLOYER_PROTO_HDRS}
        ${DEPLOYER_PROTO_SRCS}
        )

add_dependencies(host_cpu_executor_main graphengine_protos)

target_include_directories(host_cpu_executor_main PRIVATE
        ${HOST_CPU_EXECUTOR_INC_LIST}
        )

target_compile_options(host_cpu_executor_main PRIVATE
        -g
        -fPIC
        -fno-common
        -Wextra
        -Wfloat-equal
        )

target_compile_definitions(host_cpu_executor_main PRIVATE
        google=ascend_private)

target_link_options(host_cpu_executor_main PRIVATE
        -rdynamic
        -Wl,-Bsymbolic
        -Wl,--exclude-libs,ALL)

target_link_libraries(host_cpu_executor_main PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
        -Wl,--no-as-needed
        ascend_protobuf
        slog
        alog
        error_manager
        c_sec
        runtime
        ascend_hal_stub
        msprofiler_fwk_share
        adump_server
        ge_common
        graph
        ge_executor_shared
        aicpu_scheduler_so
        static_mmpa
        dl
        rt
        -Wl,--as-needed
        )


set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS grpc_client grpc_server npu_executor_main host_cpu_executor_main OPTIONAL
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        ARCHIVE DESTINATION ${INSTALL_LIBRARY_DIR}
        RUNTIME DESTINATION ${INSTALL_LIBRARY_DIR}
        )

########################################## model_deployer ###############################
set(HELPER_COMMUNICATION_SRC_LIST
        deploy/hcom/cluster/cluster_manager.cc
        deploy/hcom/cluster/cluster_parser.cc
        deploy/hcom/cluster/cluster_server.cc
        deploy/hcom/cluster/cluster_data.cc
        deploy/hcom/cluster/cluster_client.cc
        deploy/hcom/cluster/cluster_grpc_client.cc
        deploy/hcom/cluster/cluster_service_impl.cc
        deploy/hcom/rank_parser.cc
        deploy/hcom/communication_domain.cc
        )

set(MODEL_DEPLOYER_SRCS
        ${HELPER_COMMUNICATION_SRC_LIST}
        deploy/rpc/deployer_client.cc
        deploy/resource/device_info.cc
        deploy/resource/node_info.cc
        deploy/deployer/deployer.cc
        deploy/deployer/deployer_proxy.cc
        deploy/resource/resource_manager.cc
        )

add_library(model_deployer SHARED
        ${MODEL_DEPLOYER_SRCS}
        ${GRPC_PROTO_HDRS}
        ${GRPC_PROTO_SRCS}
        ${CLUSTER_PROTO_HDRS}
        ${CLUSTER_PROTO_SRCS}
        )

include_directories(
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/graph
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/inc/external
        ${CMAKE_BINARY_DIR}/proto_grpc/deployer
        ${CMAKE_CURRENT_SOURCE_DIR}
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/open_source/json/include>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc/aicpu>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/npuruntime/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abln/msprof/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/air/inc/framework>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protobuf_build-prefix/src/protobuf_build/src/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build/include/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc/mmpa>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/inc/framework>
)

target_include_directories(model_deployer PRIVATE
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/graph
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/inc/external
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos/proto
        ${CMAKE_BINARY_DIR}/proto_grpc/deployer
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${HELPER_COMMUNICATION_INC_LIST}
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/open_source/json/include>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/inc/aicpu>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/ace/npuruntime/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl/msprof/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/air/inc/framework>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/metadef>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protobuf_build-prefix/src/protobuf_build/src/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build/include/>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${METADEF_DIR}/third_party/fwkacllib/inc/mmpa>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/metadef>
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/inc/framework>
        )

target_compile_options(model_deployer PRIVATE
        -O2
        -fPIC
        -fno-common
        -Wextra
        -Wfloat-equal
        )

add_dependencies(model_deployer graphengine_protos)

target_compile_definitions(model_deployer PRIVATE
        google=ascend_private
        )

target_link_options(model_deployer PRIVATE
        -rdynamic
        -Wl,-Bsymbolic
        -Wl,--exclude-libs,ALL
        )

target_link_libraries(model_deployer PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        -Xlinker "-("
        grpc++
        grpc
        -Xlinker "-)"
        -Wl,--no-as-needed
        slog
        static_mmpa
        runtime
        c_sec
        ge_common
        error_manager
        dgw_client_so
        dl
        -Wl,--as-needed
        json
        )

set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)