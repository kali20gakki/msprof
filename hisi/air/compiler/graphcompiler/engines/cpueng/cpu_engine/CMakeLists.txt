cmake_minimum_required(VERSION 3.14)
project(cpu_engine)
if (BUILD_OPEN_PROJECT)
    get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." ABSOLUTE)
    set(PROTO_PATH ${TOP_DIR}/third_party/metadef)
else()
    get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE)
    set(PROTO_PATH ${TOP_DIR}/metadef)
endif()

set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(proto_src_files
    ${PROTO_PATH}/proto/task.proto
)

protobuf_generate(aicpu PROTO_SRCS PROTO_HDRS ${proto_src_files})

set(local_aicpu_src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_engine/engine/aicpu_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_engine/kernel_info/aicpu_kernel_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_engine/kernel_info/aicpu_cust_kernel_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_engine/optimizer/aicpu_optimizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_engine/kernel_builder/aicpu_kernel_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/aicpu_engine/kernel_builder/aicpu_ascend_ops_kernel_builder.cpp
    ${PROTO_HDRS}
)

set(local_hostcpu_src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/hostcpu_engine/engine/hostcpu_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hostcpu_engine/kernel_info/hostcpu_kernel_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hostcpu_engine/optimizer/hostcpu_optimizer.cpp
    ${PROTO_HDRS}
)

set(local_hostcpu_builder_src_files
    ${CMAKE_CURRENT_SOURCE_DIR}/hostcpu_engine/kernel_builder/hostcpu_kernel_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hostcpu_engine/kernel_builder/hostcpu_ops_kernel_builder.cpp
    ${PROTO_HDRS}
)

if (BUILD_OPEN_PROJECT)
  set(local_lib_inc_path
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/third_party/metadef
    ${TOP_DIR}/third_party/metadef/inc
    ${TOP_DIR}/third_party/metadef/inc/external
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/ops
    ${C_SEC_INCLUDE}
    ${JSON_INCLUDE}
    ${JSON_INCLUDE_DIR}
    ${Protobuf_INCLUDE}
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
  )
else()
  set (local_lib_inc_path
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/air/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/air/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/inc
    ${TOP_DIR}/metadef/inc
    ${TOP_DIR}/graphengine/inc
    ${TOP_DIR}/inc/external
    ${TOP_DIR}/libc_sec/include
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/metadef/inc/external
    ${TOP_DIR}/graphengine/inc/external
    ${TOP_DIR}/graphengine/inc/framework
    ${TOP_DIR}/third_party/json/include
    ${TOP_DIR}/open_source/json/include
    ${TOP_DIR}/metadef/graph
    ${TOP_DIR}/inc/aicpu/cpu_kernels
    ${TOP_DIR}/inc/external/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
  )
endif()

add_library(aicpu_ascend_engine SHARED
    ${local_aicpu_src_files}
)

add_library(host_cpu_engine SHARED
    ${local_hostcpu_src_files}
)

add_library(host_cpu_opskernel_builder SHARED
    ${local_hostcpu_builder_src_files}
)

target_include_directories(aicpu_ascend_engine PRIVATE
    ${local_lib_inc_path}
)

target_include_directories(host_cpu_engine PRIVATE
    ${local_lib_inc_path}
)

target_include_directories(host_cpu_opskernel_builder PRIVATE
    ${local_lib_inc_path}
)

target_compile_options(aicpu_ascend_engine PRIVATE
    -Wno-deprecated-declarations
    -std=c++11
    -fno-common
    -fno-strict-aliasing
    -Werror
)

target_compile_options(host_cpu_engine PRIVATE
    -Wno-deprecated-declarations
    -std=c++11
    -fno-common
    -fno-strict-aliasing
    -Werror
)

target_compile_options(host_cpu_opskernel_builder PRIVATE
    -Wno-deprecated-declarations
    -std=c++11
    -fno-common
    -fno-strict-aliasing
    -Werror
)

target_compile_definitions(aicpu_ascend_engine PRIVATE
    AICPU_PLUGIN
    google=ascend_private
)

target_compile_definitions(host_cpu_engine PRIVATE
    AICPU_PLUGIN
    google=ascend_private
)

target_compile_definitions(host_cpu_opskernel_builder PRIVATE
    AICPU_PLUGIN
    google=ascend_private
)

target_link_libraries(aicpu_ascend_engine PRIVATE
    -Wl,--no-as-needed
    slog
    error_manager
    graph
    platform
    ascend_protobuf
    aicpu_engine_common
    -Wl,--as-needed
    -lrt
    -ldl
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
)

target_link_libraries(host_cpu_engine PRIVATE
    -Wl,--no-as-needed
    slog
    error_manager
    graph
    platform
    ascend_protobuf
    aicpu_engine_common
    -Wl,--as-needed
    -lrt
    -ldl
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
)

target_link_libraries(host_cpu_opskernel_builder PRIVATE
    -Wl,--no-as-needed
    slog
    error_manager
    graph
    platform
    ascend_protobuf
    aicpu_engine_common
    register
    -Wl,--as-needed
    -lrt
    -ldl
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
    $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:cce_headers>>
)

set(INSTALL_LIBRARY_DIR lib)
#####改名称用的，因为一个cmake不能2个相同的名字的链接so 可执行程序 等

install(TARGETS aicpu_ascend_engine OPTIONAL
    EXPORT aicpu_ascend_engine-targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

install(TARGETS host_cpu_engine OPTIONAL
    EXPORT host_cpu_engine-targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

install(TARGETS host_cpu_opskernel_builder OPTIONAL
    EXPORT host_cpu_opskernel_builder-targets
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)
