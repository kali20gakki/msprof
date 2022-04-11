protobuf_generate(profiler
    CppPath
    HeaderPath
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/profiler.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/profiler_ext.proto
)

############################## depend ############################
add_library(alog SHARED
    ${TOP_DIR}/collector/dvvp/depend/stub/dlog_stub.cpp
)

set_target_properties(alog
    PROPERTIES
    OUTPUT_NAME alog
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)
###################################################################

set(msprofBinHeader
    include
    ../
    ../app
    ${TOP_DIR}/collector/dvvp/depend/inc/mmpa
    ${TOP_DIR}/opensource/securec/include
    ${TOP_DIR}/collector/dvvp/depend/inc/slog
    ${TOP_DIR}/opensource/json/include
    ${TOP_DIR}/collector/dvvp/depend/inc/metadef/error_manager/error_manager_stub
    ${TOP_DIR}/collector/dvvp/depend/inc
    ${TOP_DIR}/collector/dvvp/depend/inc/driver
    ${TOP_DIR}/collector/dvvp/depend/inc/adump
    ${TOP_DIR}/collector/dvvp/depend/inc/plugin
    ${TOP_DIR}/inc/toolchain
    ${TOP_DIR}/collector/dvvp/common/config
    ../common/utils
    ../msprof/engine/include
    ../host/include/
    ../message/
    ../common
    ../common/logger
    ../common/queue
    ../common/statistics
    ../common/validation
    ../common/platform
    ../depend/include/stub
    ../msprof/common/include/
    ../job_wrapper/include
    ../adx/inc
    ../driver/inc
    ../task_handle/include
    ../transport
    ../adx/inc
    ${CMAKE_BINARY_DIR}/proto/profiler/
)

set(hdcCpp
    ../adx/stub/adx_prof_api.cpp
    ../adx/wrapper/hdc_api.cpp
    ../adx/wrapper/memory_utils.cpp
)

add_executable(msprofbin
    src/msprof_bin.cpp
    src/msprof_manager.cpp
    src/prof_task.cpp
    src/data_handle.cpp
    src/prof_params_adapter.cpp
    src/cmd_log.cpp
    src/input_parser.cpp
    src/running_mode.cpp
    ../app/application.cpp
    ../app/env_manager.cpp
    ../common/utils/utils.cpp
    ../common/config/config_manager.cpp
    ../common/memory/chunk_pool.cpp
    ../common/thread/thread.cpp
    ../common/thread/thread_pool.cpp
    ../common/statistics/perf_count.cpp
    ../common/validation/param_validation.cpp
    ../common/platform/platform.cpp
    ../depend/stub/error_manager/msprof_error_manager_stub.cpp
    ../driver/devmgmt/ai_drv_dev_api.cpp
    ../driver/devmgmt/ai_drv_dsmi_api.cpp
    ../driver/channel/ai_drv_prof_api.cpp
    ../job_wrapper/src/collection_register.cpp
    ../job_wrapper/src/job_factory.cpp
    ../job_wrapper/src/job_device_rpc.cpp
    ../job_wrapper/src/prof_job.cpp
    ../job_wrapper/src/prof_peripheral_job.cpp
    ../job_wrapper/src/prof_timer.cpp
    ../job_wrapper/src/prof_host_job.cpp
    ../job_wrapper/src/prof_channel_manager.cpp
    ../job_wrapper/src/job_device_soc.cpp
    ../message/codec.cpp
    ../transport/transport.cpp
    ../transport/file_transport.cpp
    ../transport/prof_channel.cpp
    ../transport/uploader_mgr.cpp
    ../transport/hash_data.cpp
    ../transport/uploader.cpp
    ../transport/file_ageing.cpp
    ../transport/file_slice.cpp
    ../task_handle/src/info_json.cpp
    ../task_handle/src/task_relationship_mgr.cpp
    ../transport/hdc/hdc_transport.cpp
    ../transport/hdc/device_transport.cpp
    ../transport/hdc/platform/rpc/dev_mgr_api.cpp
    ../depend/src/mmpa_plugin.cpp
    ../depend/src/plugin_manager.cpp
    ../depend/src/driver_plugin.cpp
    ${hdcCpp}
    ${CppPath}
)

target_include_directories(msprofbin PRIVATE
    ${msprofBinHeader}
)


target_compile_options(msprofbin PRIVATE
    -std=c++11
    -fPIE
    -DLOG_CPP
    -D_GLIBCXX_USE_CXX11_ABI=0
    -fno-strict-aliasing
    -fno-common
)

target_compile_definitions(msprofbin PRIVATE
    SECUREC_USING_STD_SECURE_LIB=0
)

target_link_options(msprofbin PRIVATE
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
    -ldl
)

target_link_libraries(msprofbin PRIVATE
    alog
    securec_share
    protobuf
    -lpthread
    dl
)

set_target_properties(msprofbin
    PROPERTIES
    OUTPUT_NAME msprof
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

install(TARGETS msprofbin OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

