set(BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../)
set(INSTALL_LIBRARY_DIR lib)

############### adda ###################
set(addaCppList
    ${BASE_DIR}/hdc-common/ide_daemon.cpp
    ${BASE_DIR}/hdc-common/ide_daemon_hdc.cpp
    ${BASE_DIR}/hdc-common/hdc_api.cpp
    ${BASE_DIR}/hdc-common/ide_hdc_interface.cpp
    ${BASE_DIR}/hdc-common/config/config_utils.cpp
    ${BASE_DIR}/hdc-common/config/adx_config_manager.cpp
    ${BASE_DIR}/hdc-common/ide_common_util.cpp
    ${BASE_DIR}/hdc-common/ide_process_util.cpp
    ${BASE_DIR}/hdc-common/ide_platform_util.cpp
    ${BASE_DIR}/hdc-common/common/thread.cpp
    ${BASE_DIR}/hdc-common/ide_daemon_monitor.cpp
    ${BASE_DIR}/hdc-common/ide_task_register.cpp
    ${BASE_DIR}/hdc-common/common/utils.cpp
    ${BASE_DIR}/hdc-common/device/adx_dsmi.cpp
    ${BASE_DIR}/hdc-common/common/file_utils.cpp
    ${BASE_DIR}/hdc-common/common/string_utils.cpp
    ${BASE_DIR}/hdc-common/common/memory_utils.cpp)

set(addaHeaderList
    ${TOP_DIR}/abl/libc_sec/include
    ${TOP_DIR}/inc/driver
    ${BASE_DIR}/hdc-common
    ${BASE_DIR}/hdc-common/common
    ${BASE_DIR}/hdc-common/component/dump
    ${BASE_DIR}/hdc-common/platform/include
    ${BASE_DIR}/hdc-common/platform
)

add_executable(adda
    ${addaCppList}
)

set_target_properties(adda
    PROPERTIES
    OUTPUT_NAME adda
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
)

target_include_directories(adda PRIVATE
    ${addaHeaderList}
)

target_compile_options(adda PRIVATE
    -Werror
    -std=c++11
    -fPIE
    -fstack-protector-strong
    -fno-common
    -fno-strict-aliasing
)

target_link_options(adda PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
)

set(localCflags
    -DIDE_DAEMON_DEVICE
    -DOS_TYPE=0
    google=ascend_private
)

set(localSharedLibraries
    c_sec
    ascend_hal
    ascend_protobuf
    slog
    mmpa
    hdclog_device
    devprof
    adcore
)

if(PRODUCT)
    if(${PRODUCT} STREQUAL "ascend310rc")
        list(APPEND localCflags -DIDE_DAEMON_RC)
        list(APPEND localSharedLibraries stackcore ascend_monitor)
    elseif(${PRODUCT} STREQUAL "ascend310")
        list(APPEND localCflags -DPLATFORM_MONITOR)
        list(APPEND localSharedLibraries stackcore ascend_monitor)
    elseif(${PRODUCT} STREQUAL "ascend320")
        list(APPEND localCflags -DPLATFORM_MONITOR)
        list(APPEND localSharedLibraries stackcore ascend_monitor)
    elseif(${PRODUCT} STREQUAL "ascend320esl")
        list(APPEND localCflags -DPLATFORM_MONITOR)
    elseif(${PRODUCT} STREQUAL "ascend320emu")
        list(APPEND localCflags -DPLATFORM_MONITOR)
    elseif(${PRODUCT} STREQUAL "ascend320rc")
        list(APPEND localCflags -DPLATFORM_MONITOR)
    elseif(${PRODUCT} STREQUAL "ascend320rcesl")
        list(APPEND localCflags -DPLATFORM_MONITOR)
    elseif(${PRODUCT} STREQUAL "ascend320rcemu")
        list(APPEND localCflags -DPLATFORM_MONITOR)
        list(APPEND localSharedLibraries stackcore ascend_monitor)
    elseif(${PRODUCT} STREQUAL "ascend710")
        list(APPEND localCflags -DPLATFORM_MONITOR)
        list(APPEND localSharedLibraries stackcore ascend_monitor)
    elseif(${PRODUCT} STREQUAL "ascend910")
        list(APPEND localCflags -DPLATFORM_MONITOR -DUSE_TRUSTED_BASE_PATH)
        list(APPEND localSharedLibraries stackcore ascend_monitor)
    endif()
endif()

target_compile_definitions(adda PRIVATE
    ${localCflags}
)

target_link_libraries(adda PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<BUILD_INTERFACE:mmpa_headers>
    $<BUILD_INTERFACE:adump_headers>
    $<BUILD_INTERFACE:slog_headers>
    -Wl,--no-as-needed
    ${localSharedLibraries}
    -Wl,--as-needed
)

install(TARGETS adda OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

############### cfg and cert files ###################
if(${TARGET_SYSTEM_NAME} STREQUAL "Android")
set(setupShSrc ${CMAKE_CURRENT_SOURCE_DIR}/../etc/lhisi/android/setup_ide_daemon.sh)
else()
set(setupShSrc ${CMAKE_CURRENT_SOURCE_DIR}/../etc/lhisi/setup_ide_daemon.sh)
endif()
add_custom_target(AddaCfgCertFiles
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_daemon.cfg ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_daemon_cacert.pem ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_daemon_client_cert.pem ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_daemon_client_key.pem ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_daemon_server_cert.pem ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_daemon_server_key.pem ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_cmd.sh ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/ide_daemon_monitor.sh ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/host/mdc_ide_daemon.service ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/host/mdc_ide_daemon_start.sh ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/host/mdc_ide_daemon_host_install.sh ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../etc/device/ada.yaml ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${setupShSrc} ${CMAKE_CURRENT_BINARY_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ide_daemon.cfg
    ${CMAKE_CURRENT_BINARY_DIR}/ide_daemon_cacert.pem
    ${CMAKE_CURRENT_BINARY_DIR}/ide_daemon_client_cert.pem
    ${CMAKE_CURRENT_BINARY_DIR}/ide_daemon_client_key.pem
    ${CMAKE_CURRENT_BINARY_DIR}/ide_daemon_server_cert.pem
    ${CMAKE_CURRENT_BINARY_DIR}/ide_daemon_server_key.pem
    ${CMAKE_CURRENT_BINARY_DIR}/ide_cmd.sh
    ${CMAKE_CURRENT_BINARY_DIR}/ide_daemon_monitor.sh
    ${CMAKE_CURRENT_BINARY_DIR}/host/mdc_ide_daemon.service
    ${CMAKE_CURRENT_BINARY_DIR}/host/mdc_ide_daemon_start.sh
    ${CMAKE_CURRENT_BINARY_DIR}/host/mdc_ide_daemon_host_install.sh
    ${CMAKE_CURRENT_BINARY_DIR}/device/ada.yaml
    ${CMAKE_CURRENT_BINARY_DIR}/setup_ide_daemon.sh
    DESTINATION ${INSTALL_LIBRARY_DIR} OPTIONAL
)

if(PRODUCT)
    if(NOT (${PRODUCT} STREQUAL ""))
        include(libs/adump_device.cmake)
    endif()
endif()

