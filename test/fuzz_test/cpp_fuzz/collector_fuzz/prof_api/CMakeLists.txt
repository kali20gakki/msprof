protobuf_generate(profiler
    CppPath
    HeaderPath
    ${MSPROF_SOURCE_DIR}/dvvp/proto/msprofiler.proto
)

set(localSrcFiles
    test/main.cpp
    test/acl_api_test.cpp
)

aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/adx/stub SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/analyze/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/config SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/memory SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/logger SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/mmpa SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/platform SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/statistics SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/thread SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/utils SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/common/validation SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/depend/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/driver/channel SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/driver/devmgmt SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/message SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/msprof/common/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/msprof/dynamic_profiling/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/msprof/engine/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/msprof/msproftx/mstx/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/msprof/platform/mini SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/task_handle/src SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/transport SOURCE)
aux_source_directory(${MSPROF_SOURCE_DIR}/dvvp/params_adapter SOURCE)

list(REMOVE_ITEM SOURCE "${MSPROF_SOURCE_DIR}/dvvp/transport/receiver.cpp")
list(REMOVE_ITEM SOURCE "${MSPROF_SOURCE_DIR}/dvvp/adx/wrapper/adx_prof_api.cpp")
list(REMOVE_ITEM SOURCE "${MSPROF_SOURCE_DIR}/dvvp/job_wrapper/src/job_device_rpc.cpp")
list(REMOVE_ITEM SOURCE "${MSPROF_SOURCE_DIR}/dvvp/params_adapter/params_adapter_msprof.cpp")

add_executable(collector_fuzz
    ${CppPath}
    ${SOURCE}
    ${MSPROF_SOURCE_DIR}/dvvp/transport/hdc/hdc_transport.cpp
    ${MSPROF_SOURCE_DIR}/dvvp/depend/stub/error_manager/msprof_error_manager_stub.cpp
    ${localSrcFiles}
)
 
target_include_directories(collector_fuzz PRIVATE
    ${TOP_DIR}/collector/dvvp
    ${TOP_DIR}/collector/dvvp/adx/inc
    ${TOP_DIR}/collector/dvvp/analyze/inc
    ${TOP_DIR}/collector/dvvp/common
    ${TOP_DIR}/collector/dvvp/common/config
    ${TOP_DIR}/collector/dvvp/common/errno
    ${TOP_DIR}/collector/dvvp/common/utils
    ${TOP_DIR}/collector/dvvp/common/mmpa
    ${TOP_DIR}/collector/dvvp/common/logger
    ${TOP_DIR}/collector/dvvp/common/platform
    ${TOP_DIR}/collector/dvvp/common/queue
    ${TOP_DIR}/collector/dvvp/common/singleton
    ${TOP_DIR}/collector/dvvp/common/validation
    ${TOP_DIR}/collector/dvvp/depend/inc
    ${TOP_DIR}/collector/dvvp/depend/inc/acl
    ${TOP_DIR}/collector/dvvp/depend/inc/driver
    ${TOP_DIR}/collector/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${TOP_DIR}/collector/dvvp/depend/inc/plugin
    ${TOP_DIR}/collector/dvvp/depend/inc/slog
    ${TOP_DIR}/collector/dvvp/depend/inc/profapi
    ${TOP_DIR}/collector/dvvp/driver/inc
    ${TOP_DIR}/collector/dvvp/job_wrapper/include
    ${TOP_DIR}/collector/dvvp/message
    ${TOP_DIR}/collector/dvvp/msprof/common/include
    ${TOP_DIR}/collector/dvvp/msprof/dynamic_profiling/include
    ${TOP_DIR}/collector/dvvp/msprof/engine/include
    ${TOP_DIR}/collector/dvvp/msprof/msproftx/include
    ${TOP_DIR}/collector/dvvp/msprof/msproftx/mstx/inc
    ${TOP_DIR}/collector/dvvp/msprof/platform/include
    ${TOP_DIR}/collector/dvvp/params_adapter
    ${TOP_DIR}/collector/dvvp/task_handle/include
    ${TOP_DIR}/collector/dvvp/transport
    ${TOP_DIR}/collector/inc
    ${TOP_DIR}/collector/inc/external
    ${TOP_DIR}/collector/inc/external/acl
    ${TOP_DIR}/collector/inc/toolchain
    ${TOP_DIR}/platform/securec/include
)

target_compile_definitions(collector_fuzz PRIVATE
    google=ascend_private
)

target_link_libraries(collector_fuzz PRIVATE
    ${LIB_SECODE_FUZZ}
    ${SECUREC_LIB_DIR}
    ${ERROR_MANAGER_LIB_DIR}
    ascend_protobuf_static
    dl
)