project(ffts_st)

if (BUILD_OPEN_PROJECT)
    SET(FFTS_ROOT_PATH ${AIR_CODE_DIR}/compiler/graphcompiler/engines/fftseng)
	SET(FE_ROOT_PATH ${AIR_CODE_DIR}/compiler/graphcompiler/engines/nneng)
else()
    SET(FFTS_ROOT_PATH ${TOP_DIR}/air/compiler/graphcompiler/engines/fftseng)
	SET(FE_ROOT_PATH ${AIR_CODE_DIR}/compiler/graphcompiler/engines/nneng)
    SET(METADEF_DIR ${TOP_DIR}/metadef)
endif()

file(GLOB_RECURSE PROTO_LIST RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "${METADEF_DIR}/proto/task.proto"
        "${METADEF_DIR}/proto/ge_ir.proto"
        "${METADEF_DIR}/proto/om.proto"
        "${METADEF_DIR}/proto/insert_op.proto")

protobuf_generate(ffts_st PROTO_SRCS PROTO_HDRS ${PROTO_LIST})

# compile for ffts_st
add_executable(ffts_st
        ${PROTO_HDRS}
		${FE_ROOT_PATH}/utils/common/graph_comm.cc
		${FE_ROOT_PATH}/utils/common/graph_comm_impl.cc
        ${CMAKE_CURRENT_LIST_DIR}/testcase/main.cc
		${FFTS_ROOT_PATH}/common/ffts_utils.cc
		${FFTS_ROOT_PATH}/common/plugin_manager.cc
		${FFTS_ROOT_PATH}/common/ffts_constants.cc
		${FFTS_ROOT_PATH}/engine/engine_manager.cc
		${FFTS_ROOT_PATH}/engine/fftsplus_engine.cc
		${FFTS_ROOT_PATH}/optimizer/cache_optimizer/cache_manager.cc
		${FFTS_ROOT_PATH}/optimizer/graph_optimizer/fftsplus_graph_optimizer.cc
		${FFTS_ROOT_PATH}/task_builder/data_ctx/cache_persistent_manual_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/data_ctx/out_auto_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/data_ctx/out_dynamic_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/data_ctx/out_manual_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/data_ctx/prefetch_auto_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/data_ctx/prefetch_dynamic_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/data_ctx/prefetch_manual_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/fftsplus_ops_kernel_builder.cc
		${FFTS_ROOT_PATH}/task_builder/fftsplus_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/mode/auto/auto_thread_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/mode/data_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/mode/manual/manual_thread_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/mode/mixl2/mixl2_mode_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/mode/memory_slice.cc
		${FFTS_ROOT_PATH}/task_builder/mode/thread_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/aicpu_manual_task_builder.cc
        ${FFTS_ROOT_PATH}/task_builder/thread_ctx/aicpu_auto_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/aic_aiv_auto_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/aic_aiv_manual_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/collection_ops_manual_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/mix_aic_aiv_auto_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/mix_aic_aiv_manual_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/runtime_ops_manual_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/mixl2_ctx/mix_l2_task_builder.cc
		${FFTS_ROOT_PATH}/task_builder/thread_ctx/aic_aiv_dynamic_task_builder.cc
		${FFTS_ROOT_PATH}/executor/ffts_plus_update/ffts_plus_aicore_update.cc
		${FFTS_ROOT_PATH}/executor/ffts_plus_update/ffts_plus_engine_update.cc
        ${CMAKE_CURRENT_LIST_DIR}/../graph_constructor/graph_builder_utils.cc
        ${CMAKE_CURRENT_LIST_DIR}/../graph_constructor/graph_constructor.cc
        ${CMAKE_CURRENT_LIST_DIR}/../stub/slog_stub.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/engine/engine_manager_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/graph_optimizer/cache_persistent_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/main.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/aic_aiv_task_builder_stest.cc
        ${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/aicpu_auto_task_builder_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/data_context_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/function_op_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/mix_aic_aiv_auto_task_builder_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/fftsplus_ops_kernel_builder_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/fftsplus_task_builder_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/fftsplus_dynamic_task_builder_stest.cc
		${CMAKE_CURRENT_LIST_DIR}/testcase/task_builder/fftsplus_update_stest.cc
		)

if (BUILD_OPEN_PROJECT)
    target_include_directories(ffts_st PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${FFTS_ROOT_PATH}
			${METADEF_DIR}
            ${METADEF_DIR}/inc/common/opskernel
            ${METADEF_DIR}/inc/common/optimizer
            ${METADEF_DIR}/inc/common/util
            ${METADEF_DIR}/inc/common
            ${METADEF_DIR}/inc/register
            ${METADEF_DIR}/inc/external
            ${METADEF_DIR}/inc/external/graph
            ${METADEF_DIR}/inc
            ${METADEF_DIR}/inc/graph/utils
            ${METADEF_DIR}/inc/graph/
            ${METADEF_DIR}/third_party
            ${AIR_ROOT_INC_DIR}/external
            ${AIR_ROOT_INC_DIR}/framework
            ${AIR_ROOT_INC_DIR}
            ${GTEST_INCLUDE}
			${AIR_CODE_DIR}/compiler/graphcompiler/engines
			${AIR_CODE_DIR}/compiler/graphcompiler/engines/nneng/inc
			${AIR_CODE_DIR}/compiler/graphcompiler/engines/nneng/utils
            ${AIR_CODE_DIR}/test/engines/fftseng/graph_constructor
            ${CMAKE_BINARY_DIR}/proto/ffts_st)

    target_compile_options(ffts_st PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
            -D__OPTIMIZER_KT_TEST__
            -DDAVINCI_CLOUD
            -Dgoogle=ascend_private)

    target_link_libraries(ffts_st PRIVATE
            $<BUILD_INTERFACE:intf_pub>
            -Wl,--no-as-needed
            -lrt -ldl -lpthread -lgcov
            graph
            register
            ascend_protobuf
            c_sec
            platform
            error_manager
            json
            gtest
            gtest_main
            c_sec_static)
else()
    target_include_directories(ffts_st PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}
            ${FFTS_ROOT_PATH}
			${METADEF_DIR}
            ${METADEF_DIR}/inc/common/opskernel
            ${METADEF_DIR}/inc/common/optimizer
            ${METADEF_DIR}/inc/common/util
            ${METADEF_DIR}/inc/common
            ${METADEF_DIR}/inc/register
            ${METADEF_DIR}/inc/external
            ${METADEF_DIR}/inc/external/graph
            ${METADEF_DIR}/inc
            ${METADEF_DIR}/inc/graph/utils
            ${METADEF_DIR}/inc/graph/
            ${TOP_DIR}/inc/external
            ${TOP_DIR}/inc/framework
            ${TOP_DIR}/inc/ops
            ${TOP_DIR}/inc
            ${TOP_DIR}/graphengine/inc/framework
            ${TOP_DIR}/graphengine/inc/external
            ${TOP_DIR}/graphengine/inc
			${AIR_CODE_DIR}/compiler/graphcompiler/engines
			${AIR_CODE_DIR}/compiler/graphcompiler/engines/nneng/inc
			${AIR_CODE_DIR}/compiler/graphcompiler/engines/nneng/utils
            ${TOP_DIR}/air/test/engines/fftseng/graph_constructor
            ${CMAKE_BINARY_DIR}/proto/ffts_st)

    target_compile_options(ffts_st PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
            -D__OPTIMIZER_KT_TEST__
            -DDAVINCI_CLOUD
            -Dgoogle=ascend_private)

    target_link_libraries(ffts_st PRIVATE
            $<BUILD_INTERFACE:intf_llt_pub>
            $<BUILD_INTERFACE:mmpa_headers>
            $<BUILD_INTERFACE:msprof_headers>
            $<BUILD_INTERFACE:slog_headers>
	    $<BUILD_INTERFACE:runtime_headers>
	    $<BUILD_INTERFACE:tensor_engine_headers>
	    $<BUILD_INTERFACE:graph_tuner_headers>
	    $<BUILD_INTERFACE:cce_headers>
            -Wl,--no-as-needed
            -lrt -ldl -lpthread -lgcov
            graph
            register
            ascend_protobuf
            c_sec
            platform
            error_manager
            json
            c_sec_static)

    run_llt_test(
            TARGET ffts_st
            TASK_NUM 1)
endif()
