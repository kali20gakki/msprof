project(ffts)

file(GLOB_RECURSE PROTO_LIST RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "${METADEF_DIR}/proto/task.proto"
        "${METADEF_DIR}/proto/ge_ir.proto"
        )

protobuf_generate(ffts_proto PROTO_SRCS PROTO_HDRS ${PROTO_LIST} TARGET)
file(GLOB FFTS_SRC_CPP RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "engine/*.cc"
        "common/*.cc"
        "optimizer/graph_optimizer/*.cc"
        "optimizer/cache_optimizer/*.cc"
        "task_builder/*.cc"
        "task_builder/mode/*.cc"
        "task_builder/mode/auto/*.cc"
        "task_builder/mode/manual/*.cc"
        "task_builder/mode/mixl2/*.cc"
        "task_builder/data_ctx/*.cc"
        "task_builder/thread_ctx/*.cc"
        "task_builder/mixl2_ctx/*.cc")

#compiler for host
add_library(ffts SHARED
        ${FFTS_SRC_CPP}
        ${PROTO_HDRS})

add_dependencies(ffts ffts_proto)
target_include_directories(ffts PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${AIR_ROOT_INC_DIR}/framework
        ${METADEF_DIR}
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
        ${ENG_DIR}/
        ${ENG_DIR}/nneng/inc
        ${CMAKE_BINARY_DIR}/proto/ffts_proto)

target_compile_options(ffts PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private
        -Wno-deprecated-declarations)

target_compile_definitions(ffts PRIVATE

        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(ffts PRIVATE
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
        ascend_protobuf
        error_manager
        aicore_utils
        -O2
        -Wl,--no-as-needed
        json
        -ldl)

#compiler for atc host
add_library(atc_ffts SHARED
        ${FFTS_SRC_CPP}
        ${PROTO_HDRS})

set_target_properties(atc_ffts
        PROPERTIES
        OUTPUT_NAME ffts
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/atclib)

add_dependencies(atc_ffts ffts_proto)
target_include_directories(atc_ffts PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${AIR_ROOT_INC_DIR}/framework
        ${METADEF_DIR}
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
        ${CMAKE_BINARY_DIR}/proto/ffts_proto)


target_compile_definitions(atc_ffts PRIVATE
    COMPILE_OMG_PACKAGE
    )

target_compile_definitions(atc_ffts PRIVATE
    $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
    )
target_compile_options(atc_ffts PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private
        -Wno-deprecated-declarations)

target_link_libraries(atc_ffts PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        platform
        slog
        graph
        register
        c_sec
        ascend_protobuf
        error_manager
        atc_aicore_utils
        -Wl,--no-as-needed
        -O2
        json
        -ldl)

set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS ffts OPTIONAL
        EXPORT ffts-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )

install(TARGETS atc_ffts OPTIONAL
        EXPORT atc_ffts-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}/atclib
        )


file(GLOB FFTS_EXECUTOR_SRC_CPP_1 RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "executor/ffts_plus_update/*.cc")

#compiler for host
add_library(ffts_executor SHARED
        ${FFTS_EXECUTOR_SRC_CPP_1})

target_include_directories(ffts_executor PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${AIR_ROOT_INC_DIR}/framework
        ${METADEF_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/inc/graph
        ${ENG_DIR}/
        ${ENG_DIR}/fftseng/inc/
        ${ENG_DIR}/nneng/inc
        ${TOP_DIR}/graphengine/inc
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${TOP_DIR}/third_party/inc/qos>
        )

target_compile_options(ffts_executor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -O2
        -Werror
        -Wfloat-equal
        -fexceptions
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private)

target_compile_definitions(ffts_executor PRIVATE
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(ffts_executor PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        -Wl,--no-as-needed
        c_sec
        slog
        graph
        error_manager
        #qos_manager
        json
        -ldl)

set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS ffts_executor OPTIONAL
        EXPORT ffts_executor-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )
