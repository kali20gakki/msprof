protobuf_generate(profiler
    CppPath
    HeaderPath
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/profiler.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../proto/profiler_ext.proto
)

set(msprofBinHeader
    include
    ../
    ../app
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/open_source/json/include
    ${TOP_DIR}/ace/npuruntime/inc/
    ${TOP_DIR}/abl/msprof/inc/toolchain
    ${TOP_DIR}/metadef/inc
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

set(hdcStubCpp
    ../adx/stub_lhisi/adx_prof_api.cpp
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
    $<IF:$<STREQUAL:${PRODUCT},npuf10>,${hdcStubCpp},${hdcCpp}>
    ${CppPath}
)

target_include_directories(msprofbin PRIVATE
    ${msprofBinHeader}
)


target_compile_options(msprofbin PRIVATE
    -std=c++11
    -fPIE
    -fno-strict-aliasing
    -fno-common
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wfloat-equal>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wextra>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wno-unused-parameter>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Wno-missing-field-initializers>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(msprofbin PRIVATE
    SECUREC_USING_STD_SECURE_LIB=0
    google=ascend_private
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:NOMINMAX>
)

target_link_options(msprofbin PRIVATE
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
    -ldl
)

target_link_libraries(msprofbin PRIVATE
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    $<IF:$<STREQUAL:${PRODUCT},npuf10>,$<BUILD_INTERFACE:msprofbinNpuf10_pub>,$<BUILD_INTERFACE:msprofbinOnetrack_pub>>
)

set_target_properties(msprofbin
    PROPERTIES
    OUTPUT_NAME msprof
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

install(TARGETS msprofbin OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

### onetrack target interface

add_library(msprofbinOnetrack_pub INTERFACE)

target_link_libraries(msprofbinOnetrack_pub INTERFACE
    $<BUILD_INTERFACE:intf_pub>
    c_sec
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,alog,static_alog>
    mmpa
    $<IF:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,ascend_hal,ascend_hal_stub>
    ascend_protobuf_static
    -lpthread
)
### lhisi target interface

add_library(msprofbinNpuf10_pub INTERFACE)

target_link_libraries(msprofbinNpuf10_pub INTERFACE
    $<BUILD_INTERFACE:intf_pub>
    ascend_protobuf_static
    -Wl,--no-as-needed
    c_sec
    static_alog
    slog
    mmpa
    -Wl,--as-needed
    -ldrvdevdrv
    -ldrv_dfx
    -ldl
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:-llog>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>:-lpthread>
)

target_link_directories(msprofbinNpuf10_pub INTERFACE
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:${TOP_DIR}/vendor/sdk/hi3796/drv_android>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:${TOP_DIR}/build/prebuilts/clang/linux-x86/aarch64/android-ndk-r21/sysroot/usr/lib/aarch64-linux-android/29>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_LINUX_DISTRIBUTOR_RELEASE},100>>:${TOP_DIR}/vendor/sdk/hi3796/drv>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Linux>,$<STREQUAL:${TARGET_LINUX_DISTRIBUTOR_RELEASE},200>>:${TOP_DIR}/vendor/sdk/hi3559dv100/drv>
)

