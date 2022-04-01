project (gerunner)

##################################################################
set(RUNNER_SRC_LIST
    "client/ge_api.cc"
    "session/inner_session.cc"
    "session/session_manager.cc"
    "common/plugin/tbe_plugin_manager.cc"
    "common/profiling/profiling_init.cc"
)

if (NOT ENABLE_D AND NOT ENABLE_ACL AND NOT ENABLE_MS_TESTCASES)
message("CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
############ libge_runner.so ############
add_library(ge_runner SHARED
    ${RUNNER_SRC_LIST}
    $<TARGET_OBJECTS:$<IF:$<TARGET_EXISTS:msprofiler_fwk>,msprofiler_fwk,msprofiler_fwk_object>>
)

add_library(msprofiler_fwk_object OBJECT IMPORTED GLOBAL)

if (msprofiler_fwk_ext_LIBRARY_DIR)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_fwk_object)
    execute_process(
        COMMAND ar x ${msprofiler_fwk_ext_LIBRARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_fwk_object
    )
    file(GLOB MSPROFILER_FWK_OBJECT_LIST ${CMAKE_CURRENT_BINARY_DIR}/msprofiler_fwk_object/*.o)
    set_property(TARGET msprofiler_fwk_object PROPERTY IMPORTED_OBJECTS ${MSPROFILER_FWK_OBJECT_LIST})
endif()

target_compile_definitions(ge_runner PRIVATE
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    REUSE_MEMORY=1
    FMK_SUPPORT_DUMP
    DAVINCI_CLOUD
    google=ascend_private
    $<$<STREQUAL:${ENABLE_OPEN_SRC},True>:ONLY_COMPILE_OPEN_SRC>
)

target_compile_options(ge_runner PRIVATE
    -O2
    -fno-common
    -Werror
    -Wextra
    -Wfloat-equal
    -fvisibility=default
)

target_include_directories(ge_runner SYSTEM PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/compiler
    ${AIR_CODE_DIR}/executor
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/external
    ${AIR_CODE_DIR}/inc/framework
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto/graphengine_protos
    #### yellow zone ####
    ${GE_DEPEND_DIR}/toolchain/ide/ide-daemon/external
    ${GE_DEPEND_DIR}/abl/adump/external
    ${GE_DEPEND_DIR}/abl/licctrl
    ${GE_DEPEND_DIR}/ace/comop/inc
    ${GE_DEPEND_DIR}/ace/comop/inc/external
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:runtime_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:cce_headers,INTERFACE_INCLUDE_DIRECTORIES>>
    #### blue zone ####
    ${ASCEND_DIR}/driver/include
    ${ASCEND_DIR}/fwkacllib/include
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc/toolchain>
)

target_link_options(ge_runner PRIVATE
    -Wl,-Bsymbolic
)

target_link_libraries(ge_runner PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
    ge_compiler
    ge_executor_shared
    -Wl,--as-needed
    graph
    ge_common
    register
    runtime
    error_manager
    ascend_hal_stub
    opt_feature
    msprofiler_fwk_share
    adump_server
    static_mmpa
    slog
    c_sec
    ascend_protobuf
    json
    -Wl,--no-as-needed
    -lrt
    -ldl
)

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS ge_runner OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/engine_conf.json
    ${CMAKE_CURRENT_BINARY_DIR}/optimizer_priority.pbtxt OPTIONAL
    DESTINATION ${INSTALL_LIBRARY_DIR}
)
endif()

ADD_SUBDIRECTORY(stub)
