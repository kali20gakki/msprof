#########libadcore
set(INSTALL_LIBRARY_DIR lib)

#set(TOP_DIR ../../../../..)
set(adumpHeaders
    ${TOP_DIR}/libc_sec/include
    ${CMAKE_CURRENT_LIST_DIR}/../../external
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/log
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump
    ${TOP_DIR}/inc/driver
)

set(adumpSrcList
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/adx_comm_opt_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/memory_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_process.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_record.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/file_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol/adx_msg_proto.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/string_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_soc_helper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_datadump_server_soc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_sock_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/sock_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/sock_comm_opt.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/thread.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/ide_process_util.cpp
)

set(socSrcFiles ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_soc_api.cpp)

set(adumpNormalSrcList
    ${adumpSrcList}
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_dsmi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_hdc_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/hdc_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/ide_hdc_interface.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_hdc_helper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_hdc_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/hdc_comm_opt.cpp
)

set(adumpNpuf10SrcList
    ${adumpSrcList}
    ${socSrcFiles}
)

set(adumpRcSrcList
    ${adumpSrcList}
    ${socSrcFiles}
)

set(adumpMdcSrcList
    ${adumpSrcList}
    ${socSrcFiles}
)

add_library(adump SHARED
    $<$<STREQUAL:${PRODUCT},ascend>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend310>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend310rc>:${adumpRcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend320>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend320esl>:${adumpRcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend320emu>:${adumpRcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend320rc>:${adumpRcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend320rcesl>:${adumpRcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend320rcemu>:${adumpRcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend610>:${adumpMdcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend615>:${adumpMdcSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend710>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend910>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},ascend920>:${adumpNormalSrcList}>
    $<$<STREQUAL:${PRODUCT},npuf10>:${adumpNpuf10SrcList}>
    $<$<STREQUAL:${PRODUCT},helper710>:${adumpNormalSrcList}>
)

target_include_directories(adump PRIVATE
	${adumpHeaders}
)

target_compile_options(adump PRIVATE
    -Werror
    -std=c++11
    -fstack-protector-strong
    -fPIC
    -fno-common
    -fno-strict-aliasing
)

target_compile_definitions(adump PRIVATE
    ADX_LIB
    OS_TYPE=0
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Android>:ANDROID>
    $<$<STREQUAL:${PRODUCT},npuf10>:IDE_UNIFY_HOST_DEVICE>
    $<$<STREQUAL:${PRODUCT},ascend310>:IDE_UNIFY_HOST_DEVICE>
    $<$<STREQUAL:${PRODUCT},ascend610>:IDE_UNIFY_HOST_DEVICE>
    $<$<STREQUAL:${PRODUCT},ascend615>:IDE_UNIFY_HOST_DEVICE>
)

target_link_libraries(adump PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    -Wl,--no-as-needed
    c_sec
    mmpa
    slog
    -Wl,--as-needed
    $<$<STREQUAL:${PRODUCT},ascend>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend310>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend710>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend910>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920esl>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920emu>:ascend_hal>
    $<$<STREQUAL:${PRODUCT},ascend920>:ascend_hal>
)

target_link_options(adump PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
)

install(TARGETS adump OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)
