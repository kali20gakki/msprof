add_subdirectory(msprofbin)

protobuf_generate(profiler_proto
    CppPath
    HeaderPath
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/profiler.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/profiler_ext.proto
    TARGET
)

set(hdcCpp
    adx/stub/adx_prof_api.cpp
    adx/wrapper/hdc_api.cpp
    adx/wrapper/memory_utils.cpp
)

set(hdcStubCpp
    adx/stub_lhisi/adx_prof_api.cpp
)

############################## stub ################################
set(MSPROF_STUB_INC ${TOP_DIR}/collector/inc/external)
set(HI_PYTHON python3)
add_custom_target(
    stub_msprof_src ALL
    DEPENDS  ${CMAKE_CURRENT_BINARY_DIR}/acl_prof_stub.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/ge_prof_stub.cpp
)
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/acl_prof_stub.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/ge_prof_stub.cpp
    COMMAND echo "Generating stub files:acl_prof_stub.cpp, ge_prof_stub.cpp."
            && ${HI_PYTHON} ${TOP_DIR}/cmake/stub/gen_stubapi.py ${MSPROF_STUB_INC}
            ${CMAKE_CURRENT_BINARY_DIR}/acl_prof_stub.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/ge_prof_stub.cpp
            && echo "Generating stub files end."
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${TOP_DIR}/cmake/stub/gen_stubapi.py ${MSPROF_STUB_INC}
)
############################## stub ################################

###################### stub/libmsprofiler.so #######################

add_library(stub_msprofiler SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/acl_prof_stub.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/ge_prof_stub.cpp
)

set_target_properties(stub_msprofiler
    PROPERTIES
    OUTPUT_NAME msprofiler
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stub
)

target_include_directories(stub_msprofiler PRIVATE
    ${MSPROF_STUB_INC}
    ${TOP_DIR}/collector/dvvp/depend/inc/acl
    ${TOP_DIR}/collector/dvvp/depend/inc/graphengine/external
    ${TOP_DIR}/collector/dvvp/depend/inc/metadef
    ${TOP_DIR}/collector/inc/external
    ${TOP_DIR}/collector/dvvp/depend/inc/profapi
)

target_compile_options(stub_msprofiler PRIVATE
    -std=c++11
    -fPIC
    -fstack-protector-all
    -fno-strict-aliasing
    -fno-common
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wfloat-equal
    -Wextra
)

install(TARGETS stub_msprofiler OPTIONAL
    LIBRARY DESTINATION stub
)
###################### stub/libmsprofiler.so #######################

# ####################### libmsprofiler.so ###########################
set(msprofilerCpp
    analyze/src/analyzer.cpp
    analyze/src/analyzer_base.cpp
    analyze/src/analyzer_ge.cpp
    analyze/src/analyzer_hwts.cpp
    analyze/src/analyzer_ffts.cpp
    analyze/src/analyzer_ts.cpp
    analyze/src/op_desc_parser.cpp
    common/config/config_manager.cpp
    common/memory/chunk_pool.cpp
    common/platform/platform.cpp
    common/statistics/perf_count.cpp
    common/thread/thread.cpp
    common/thread/thread_pool.cpp
    common/utils/utils.cpp
    common/validation/param_validation.cpp
    driver/channel/ai_drv_prof_api.cpp
    driver/devmgmt/ai_drv_dev_api.cpp
    driver/devmgmt/ai_drv_dsmi_api.cpp
    job_wrapper/src/collection_register.cpp
    job_wrapper/src/job_device_soc.cpp
    job_wrapper/src/job_factory.cpp
    job_wrapper/src/prof_channel_manager.cpp
    job_wrapper/src/prof_job.cpp
    job_wrapper/src/prof_peripheral_job.cpp
    job_wrapper/src/prof_timer.cpp
    job_wrapper/src/prof_host_job.cpp
    message/codec.cpp
    msprof/common/src/receive_data.cpp
    msprof/common/src/msprof_callback_handler.cpp
    msprof/engine/src/aicpu_plugin.cpp
    msprof/engine/src/prof_api_common.cpp
    msprof/engine/src/prof_acl_mgr.cpp
    msprof/engine/src/uploader_dumper.cpp
    msprof/platform/mini/data_dumper.cpp
    msprof/msproftx/src/msprof_tx_manager.cpp
    msprof/msproftx/src/msprof_tx_reporter.cpp
    msprof/msproftx/src/prof_stamp_pool.cpp
    msprofiler/src/command_handle.cpp
    msprofiler/src/msprof_callback_impl.cpp
    task_handle/src/device.cpp
    task_handle/src/info_json.cpp
    task_handle/src/prof_manager.cpp
    task_handle/src/prof_params_adapter.cpp
    task_handle/src/prof_task.cpp
    task_handle/src/task_relationship_mgr.cpp
    transport/file_ageing.cpp
    transport/file_slice.cpp
    transport/file_transport.cpp
    transport/hdc/hdc_transport.cpp
    transport/hdc/platform/soc/dev_mgr_api.cpp
    depend/src/driver_plugin.cpp
    depend/src/mmpa_plugin.cpp
    depend/src/plugin_handle.cpp
    transport/parser_transport.cpp
    transport/prof_channel.cpp
    transport/transport.cpp
    transport/uploader.cpp
    transport/uploader_mgr.cpp
    transport/hash_data.cpp
    ${CppPath}
)

set(msprofilerInc
    ${TOP_DIR}/collector/inc
    ################################## Check #####################################
    ${TOP_DIR}/collector/dvvp/depend/inc/graphengine/external
    ${TOP_DIR}/collector/dvvp/depend/inc/graphengine
    ${TOP_DIR}/collector/dvvp/depend/inc/graphengine/framework/common
    ${TOP_DIR}/collector/dvvp/depend/inc/driver
    ${TOP_DIR}/collector/dvvp/depend/inc/adump
    ${TOP_DIR}/collector/dvvp/depend/inc/slog
    ${TOP_DIR}/collector/dvvp/depend/inc/metadef
    ${TOP_DIR}/collector/dvvp/depend/inc/acl
    ${TOP_DIR}/collector/dvvp/depend/inc/mmpa
    ${TOP_DIR}/collector/dvvp/depend/inc/slog
    ${TOP_DIR}/collector/dvvp/depend/inc/profapi
    ################################# Check #######################################
    ${TOP_DIR}/collector/inc/toolchain
    ${TOP_DIR}/collector/inc/external
    ${TOP_DIR}/collector/inc/external/acl
    ${TOP_DIR}/opensource/securec/include
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/collector/dvvp/depend/inc/plugin
    msprof/engine/include
    msprof/common/include
    msprof/drvprof/include
    msprof/platform/include
    msprof/msproftx/include
    adx/inc
    analyze/inc
    common
    common/errno
    common/logger
    common/queue
    common/singleton
    common/utils
    common/config
    common/validation
    common/statistics
    common/platform
    msprofiler/inc
    msprof/common/include
    message
    transport
    task_handle/include
    driver/inc
    job_wrapper/include
    msprofiler/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/proto/profiler_proto/
)

set(msprofilerFwcSrc
    ${msprofilerCpp}
    msprof/engine/src/prof_ge_core.cpp
)

add_library(msprofiler_fwk_share SHARED
    ${msprofilerFwcSrc}
    depend/stub/error_manager/msprof_error_manager.cpp
    msprof/engine/src/prof_acl_core.cpp
    ${hdcCpp}
)

target_include_directories(msprofiler_fwk_share PRIVATE
    ${msprofilerInc}
    ${TOP_DIR}/collector/dvvp/depend/inc/metadef/error_manager/error_manager_wrapper
    ${TOP_DIR}/collector/dvvp/depend/inc
)

set_target_properties(msprofiler_fwk_share
    PROPERTIES
    OUTPUT_NAME msprofiler
)

target_compile_definitions(msprofiler_fwk_share PRIVATE
    google=ascend_private
    LOG_CPP
)

target_compile_options(msprofiler_fwk_share PRIVATE
    -std=c++11
    -fPIC
    -fstack-protector-all
    -fno-strict-aliasing
    -fno-common
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
)

target_link_options(msprofiler_fwk_share PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
)

target_link_libraries(msprofiler_fwk_share PRIVATE
    -Wl,--no-as-needed
    c_sec
    alog
    ascend_protobuf_shared
    profapi
    error_manager
    -Wl,--as-needed
)

install(TARGETS msprofiler_fwk_share OPTIONAL
    LIBRARY DESTINATION lib
)

####################### libmsprofiler.so ###########################
add_dependencies(msprofiler_fwk_share profiler_proto)
