project(aicore_utils)

file(GLOB_RECURSE PROTO_LIST RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "${METADEF_DIR}/proto/task.proto"
        "${METADEF_DIR}/proto/ge_ir.proto"
        )

protobuf_generate(aicore_utils_proto PROTO_SRCS PROTO_HDRS ${PROTO_LIST} TARGET)

file(GLOB COMM_SRC_CPP_1 RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "adapter/adapter_itf/*.cc"
        "adapter/factory/*.cc"
        "adapter/tbe_adapter/*.cc"
        "adapter/tbe_adapter/kernel_launch/*.cc"
        "ffts/*.cc"
        "aicore_manager/*.cc"
        "common/*.cc"
        "ffts_plus_task_builder/data/*.cc"
        "cmo_task_builder/*.cc"
        "cmo_task_builder/cmo_task/*.cc"
        "ops_kernel_builder/*.cc"
        "task_builder/*.cc"
        "param_calculate/*.cc"
        "task_builder/*.cc"
        "dsa/*.cc")

#compiler for host fwkacllib
add_library(aicore_utils SHARED
        ${COMM_SRC_CPP_1}
        ${PROTO_HDRS})

add_dependencies(aicore_utils aicore_utils_proto)

target_include_directories(aicore_utils PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/graph/
        ${TOP_DIR}/graphengine/inc
        ${TOP_DIR}/graphengine/inc/external
        ${CMAKE_BINARY_DIR}/proto/aicore_utils_proto
        ${ENG_DIR})


target_compile_options(aicore_utils PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -Wfloat-equal
        -fexceptions
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private)

target_compile_definitions(aicore_utils PRIVATE
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(aicore_utils PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
        -O2
        -Wl,--no-as-needed
        c_sec
        slog
        graph
        ascend_protobuf
        error_manager
        register
        runtime
        -Wl,--no-as-needed
        json
        -ldl)

#compiler for host atc
add_library(atc_aicore_utils SHARED
        ${COMM_SRC_CPP_1}
        ${PROTO_HDRS})

set_target_properties(atc_aicore_utils
        PROPERTIES
        OUTPUT_NAME aicore_utils
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/atclib)

add_dependencies(atc_aicore_utils aicore_utils_proto)

target_include_directories(atc_aicore_utils PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/graph/
        ${TOP_DIR}/graphengine/inc
        ${TOP_DIR}/graphengine/inc/external
        ${CMAKE_BINARY_DIR}/proto/aicore_utils_proto
        ${ENG_DIR})

target_compile_definitions(atc_aicore_utils PRIVATE
        COMPILE_OMG_PACKAGE
        )

target_compile_definitions(atc_aicore_utils PRIVATE
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_compile_options(atc_aicore_utils PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -Wfloat-equal
        -O2
        -fexceptions
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private)

target_link_libraries(atc_aicore_utils PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
        -O2
        c_sec
        slog
        graph
        ascend_protobuf
        error_manager
        register
        runtime
        -Wl,--no-as-needed
        json
        -ldl)

#compiler for host acl
add_library(aicore_utils_runtime SHARED
        ${COMM_SRC_CPP_1}
        ${PROTO_HDRS})

add_dependencies(aicore_utils_runtime aicore_utils_proto)

target_include_directories(aicore_utils_runtime PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/graph/
        ${TOP_DIR}/graphengine/inc
        ${TOP_DIR}/graphengine/inc/external
        ${CMAKE_BINARY_DIR}/proto/aicore_utils_proto
        ${ENG_DIR})

target_compile_options(aicore_utils_runtime PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -Wfloat-equal
        -fexceptions
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private)

target_compile_definitions(aicore_utils_runtime PRIVATE
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(aicore_utils_runtime PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
        -O2
        c_sec
        slog
        graph
        ascend_protobuf
        error_manager
        register
        runtime
        -Wl,--no-as-needed
        json
        -ldl)

#compiler for host static
add_library(aicore_utils_static STATIC
        ${COMM_SRC_CPP_1}
        ${PROTO_HDRS})

add_dependencies(aicore_utils_static aicore_utils_proto)

target_include_directories(aicore_utils_static PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${AIR_ROOT_INC_DIR}/external
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/graph/
        ${TOP_DIR}/graphengine/inc
        ${TOP_DIR}/graphengine/inc/external
        ${CMAKE_BINARY_DIR}/proto/aicore_utils_proto
        ${ENG_DIR})

target_compile_options(aicore_utils_static PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -Wfloat-equal
        -O2
        -fexceptions
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private)

target_compile_definitions(aicore_utils_static PRIVATE
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(aicore_utils_static PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
        -O2
        c_sec
        slog
        graph
        ascend_protobuf
        error_manager
        register
        runtime
        -Wl,--no-as-needed
        json
        -ldl)


set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS aicore_utils OPTIONAL
        EXPORT aicore_utils-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )

install(TARGETS atc_aicore_utils OPTIONAL
        EXPORT atc_aicore_utils-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}/atclib
        )

install(TARGETS aicore_utils_runtime OPTIONAL
        EXPORT aicore_utils_runtime-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )

install(TARGETS aicore_utils_static OPTIONAL
        EXPORT aicore_utils_static-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )
