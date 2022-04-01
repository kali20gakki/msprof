set(MSPROF_SOURCE_DIR ${TOP_DIR}/abl/msprof/collector)

protobuf_generate(profiler
    CppPath
    HeaderPath
    ${MSPROF_SOURCE_DIR}/dvvp/proto/profiler.proto
    ${MSPROF_SOURCE_DIR}/dvvp/proto/profiler_ext.proto
)
add_subdirectory(ut)
add_subdirectory(st)
