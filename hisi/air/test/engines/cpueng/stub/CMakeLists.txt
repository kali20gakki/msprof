cmake_minimum_required(VERSION 3.14.1)
project(stub_for_aicpu)
get_filename_component(TOP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)

set(proto_src_files
    ${TOP_DIR}/third_party/metadef/proto/task.proto
)

protobuf_generate(aicpu PROTO_SRCS PROTO_HDRS ${proto_src_files})

set(engine_common_stub_files
    ${CMAKE_CURRENT_SOURCE_DIR}/platform_info_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/util/util_stub.cpp
    ${PROTO_HDRS}
)

set(engine_common_includes
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng
    ${TOP_DIR}/compiler/graphcompiler/engines/cpueng/common
    ${TOP_DIR}/compiler/graphcompiler/engines/nneng/inc
    ${TOP_DIR}/third_party/metadef
    ${TOP_DIR}/third_party/metadef/inc
    ${TOP_DIR}/third_party/metadef/inc/external
    ${TOP_DIR}/third_party/metadef/inc/graph
    ${TOP_DIR}/third_party/inc/external
    ${TOP_DIR}/third_party/inc
    ${TOP_DIR}/third_party/inc/framework
    ${TOP_DIR}/third_party/inc/framework/common/
    ${TOP_DIR}/third_party/inc/ops
    ${C_SEC_INCLUDE}
    ${JSON_INCLUDE}
    ${JSON_INCLUDE_DIR}
    ${Protobuf_INCLUDE}
    ${CMAKE_BINARY_DIR}/proto/aicpu
    ${CMAKE_BINARY_DIR}/proto/aicpu/proto
)

add_subdirectory(slog)
add_subdirectory(runtime)

add_library(stub_for_aicpu SHARED
    ${engine_common_stub_files}
)
add_dependencies(stub_for_aicpu gtest)
target_include_directories(stub_for_aicpu PRIVATE
    ${engine_common_includes}
)

target_compile_definitions(stub_for_aicpu PRIVATE
    RUN_TEST
)

target_compile_options(stub_for_aicpu PRIVATE
    -std=c++11
    -fPIC
)

target_link_libraries(stub_for_aicpu PRIVATE
    slog_aicpu_stub
    runtime_aicpu_stub
    ascend_protobuf
)

set(COPY_TO_PATH ${TOP_DIR}/out/onetrack/llt/ut/obj/lib/)
add_custom_command(TARGET stub_for_aicpu POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${COPY_TO_PATH}
                  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:stub_for_aicpu> ${COPY_TO_PATH}
                  COMMENT "[AICPU] copying stub_for_aicpu library to '${COPY_TO_PATH}'"
)
set(INSTALL_LIBRARY_DIR lib)
install(TARGETS stub_for_aicpu OPTIONAL
    EXPORT stub_for_aicpu-targets
    LIBRARY DESTINATION ${COPY_TO_PATH}
)
