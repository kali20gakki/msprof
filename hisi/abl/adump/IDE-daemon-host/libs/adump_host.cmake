#########libadcore
set(adcoreHeaders
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_LIST_DIR}/../../external
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/epoll
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/manager
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/manager/component
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component
    ${TOP_DIR}/inc/driver
)

set(adcoreSrcList 
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/hdc_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/ide_hdc_interface.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_dsmi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_hdc_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/adx_server_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/adx_file_dump.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/adx_get_file.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/epoll/adx_hdc_epoll.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/adx_comm_opt_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/hdc_comm_opt.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/thread.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/memory_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol/adx_msg_proto.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/file_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/ide_process_util.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_core_dump_server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_get_file_server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_operate_log_level.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_log_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_api.cpp
)

add_library(adcore STATIC
    ${adcoreSrcList}
)

target_include_directories(adcore PRIVATE
    ${adcoreHeaders}
)

target_compile_options(adcore PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    -std=c++11
    -fno-common
    -fno-strict-aliasing
)

target_compile_definitions(adcore PRIVATE
    $<IF:$<STREQUAL:${PRODUCT_SIDE},host>,ADX_LIB_HOST_DRV,ADX_LIB>
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
)

target_link_libraries(adcore PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    -Wl,--no-as-needed
    c_sec
    mmpa
    -Wl,--as-needed
    ascend_hal
)


########liblog_server
#set(TOP_DIR ../../../../..)
set(logServerHeaders
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_LIST_DIR}/../../external
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/epoll
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/manager
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/manager/component
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component
    ${TOP_DIR}/inc/driver
    ${TOP_DIR}/abl/slog/shared
)

set(logServerSrcList 
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/hdc_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/ide_hdc_interface.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_dsmi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_hdc_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/adx_server_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/epoll/adx_hdc_epoll.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/adx_comm_opt_manager.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/hdc_comm_opt.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/thread.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/memory_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol/adx_msg_proto.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/file_utils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/ide_process_util.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_log_hdc_server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/adx_log_hdc.cpp
)

set(no_needed_linklibs )
set(needed_linklibs )
if (PRODUCT)
    if (${PRODUCT} STREQUAL "npuf10" OR ${PRODUCT} STREQUAL "ascend310rc" OR ${TARGET_SYSTEM_NAME} STREQUAL "Windows")
        set(logServerSrcList
            ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_log_hdc_server_stub.cpp
        )
        set(logServerHeaders
            ${CMAKE_CURRENT_LIST_DIR}/../../external
            ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
        )
    else()
        list(APPEND no_needed_linklibs c_sec mmpa slog)
        list(APPEND needed_linklibs ascend_hal)
    endif ()
endif()

add_library(log_server STATIC
    ${logServerSrcList}
)

target_include_directories(log_server PRIVATE
    ${logServerHeaders}
)

target_compile_options(log_server PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    -std=c++11
    -fno-common
    -fno-strict-aliasing
)

target_compile_definitions(log_server PRIVATE
    $<IF:$<STREQUAL:${PRODUCT_SIDE},host>,ADX_LIB_HOST_DRV,ADX_LIB>
    SESSION_ACTIVE
    OS_TYPE=0
)

target_link_libraries(log_server PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    -Wl,--no-as-needed
    ${no_needed_linklibs}
    -Wl,--as-needed
    ${needed_linklibs}
)

################### libadump_server.a

set(adumpServerHeaders
    ${TOP_DIR}/abl/libc_sec/include
    ${CMAKE_CURRENT_LIST_DIR}/../../external
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/log
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/epoll
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/manager
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol
    ${TOP_DIR}/inc/driver
)

set(adumpServerStubHeaders
    ${CMAKE_CURRENT_LIST_DIR}/../../external
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
)

set(adumpServerSrcList 
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/adx_server_manager.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_receive.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/adx_comm_opt_manager.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/thread.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/memory_utils.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/file_utils.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/ide_process_util.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/common/string_utils.cpp
	${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/protocol/adx_msg_proto.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_process.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/component/dump/adx_dump_record.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/hdc_api.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_dsmi.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/device/adx_hdc_device.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/commopts/hdc_comm_opt.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common/epoll/adx_hdc_epoll.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_datadump_callback.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_datadump_server.cpp
)

set(adumpServerStubHeaders
    ${CMAKE_CURRENT_LIST_DIR}/../../external
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
)

set(adumpServerStubSrcList 
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_datadump_server_stub.cpp
)

set(linklibs )
if (PRODUCT)
    if (${PRODUCT} STREQUAL  "ascend610" OR
        ${PRODUCT} STREQUAL  "ascend615" OR
        ${PRODUCT} STREQUAL  "npuf10" OR
        ${PRODUCT} STREQUAL  "ascend310rc" OR
        (${PRODUCT} STREQUAL  "ascend310" AND ${PRODUCT_SIDE} STREQUAL "device"))
        set(adumpServerSrcList
            ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_datadump_server_stub.cpp
        )
        set(adumpServerHeaders
            ${CMAKE_CURRENT_LIST_DIR}/../../external
            ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
        )
    else()
        list(APPEND linklibs c_sec mmpa slog)
    endif ()
endif()

add_library(adump_server STATIC
    ${adumpServerSrcList}
)

target_include_directories(adump_server PRIVATE
    ${adumpServerHeaders}
)

target_compile_options(adump_server PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    -std=c++11
    -fno-common
    -fno-strict-aliasing
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Debug>>:/MTd>
    $<$<AND:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_CONFIGURATION_TYPES},Release>>:/MT>
)

target_compile_definitions(adump_server PRIVATE
    $<IF:$<STREQUAL:${PRODUCT_SIDE},host>,ADX_LIB_HOST,ADX_LIB>
    LOG_CPP
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:OS_TYPE=0>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:OS_TYPE=1>
    $<$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>:SECUREC_USING_STD_SECURE_LIB=0>
)

target_link_libraries(adump_server PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    -Wl,--no-as-needed
    ${linklibs}
    -Wl,--as-needed
)

install(TARGETS adump_server OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

###### host libadump_server_stub.a

set(adumpServerStubHeaders
    ${CMAKE_CURRENT_LIST_DIR}/../../external
    ${CMAKE_CURRENT_LIST_DIR}/../../hdc-common
)

set(adumpServerStubSrcList 
    ${CMAKE_CURRENT_LIST_DIR}/../../external/adx_datadump_server_stub.cpp
)

add_library(adump_server_stub STATIC
    ${adumpServerStubSrcList}
)

target_include_directories(adump_server_stub PRIVATE
	${adumpServerStubHeaders}
)

target_compile_options(adump_server_stub PRIVATE
    $<$<NOT:$<STREQUAL:${TARGET_SYSTEM_NAME},Windows>>:-Werror>
    -std=c++11
    -fno-common
    -fno-strict-aliasing
)

target_compile_definitions(adump_server_stub PRIVATE
    OS_TYPE=0
)

target_link_libraries(adump_server_stub PRIVATE
    $<BUILD_INTERFACE:intf_pub>
)
