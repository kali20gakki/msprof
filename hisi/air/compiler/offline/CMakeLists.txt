set(SRC_LIST
        "single_op_parser.cc"
        "omg.cc"
        "main_impl.cc"
        )

set(COMPILE_OPTIONS
        -Werror
        -Wextra
        -Wfloat-equal
        -O2
        -fno-common
        -fvisibility=hidden
        )

set(COMPILE_DEFINITIONS
        PROTOBUF_INLINE_NOT_IN_HEADERS=0
        google=ascend_private
        LOG_CPP
        FUNC_VISIBILITY
        )

############ atc_static ############
add_library(atc_static ${SRC_LIST})

add_dependencies(atc_static
        graphengine_protos
        )

target_compile_options(atc_static PRIVATE ${COMPILE_OPTIONS})
target_compile_definitions(atc_static PRIVATE ${COMPILE_DEFINITIONS})

target_include_directories(atc_static PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${AIR_CODE_DIR}/base
        ${AIR_CODE_DIR}/compiler
        ${AIR_CODE_DIR}/inc
        ${AIR_CODE_DIR}/inc/external
        ${AIR_CODE_DIR}/inc/framework
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/external
        ${PARSER_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/proto/graphengine_protos
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:runtime_headers,INTERFACE_INCLUDE_DIRECTORIES>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<TARGET_PROPERTY:cce_headers,INTERFACE_INCLUDE_DIRECTORIES>>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
        )

target_link_options(atc_static PRIVATE
        -Wl,-Bsymbolic
        -Wl,-rpath-link,${ASCEND_ATC_DIR}/stub
        -Wl,-rpath-link,${ASCEND_RUNTIME_DIR}/stub
        )

target_link_libraries(atc_static PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:mmpa_headers>>
        ascend_protobuf
        ge_common
        register
        c_sec
        graph
        error_manager
        ge_compiler
        parser_common
        gflags
        json
        runtime
        slog
	mmpa
        -lrt
        -ldl
        )

############ atc_atc.bin ############
add_executable(atc_atc.bin main.cc)

target_compile_options(atc_atc.bin PRIVATE ${COMPILE_OPTIONS})

target_compile_definitions(atc_atc.bin PRIVATE ${COMPILE_DEFINITIONS})

target_link_options(atc_atc.bin PRIVATE
        -Wl,-Bsymbolic
        -Wl,-rpath-link,${ASCEND_ATC_DIR}/stub
        -Wl,-rpath-link,${ASCEND_RUNTIME_DIR}/stub
        )

target_link_libraries(atc_atc.bin PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        atc_static
        )

set_target_properties(atc_atc.bin PROPERTIES
        OUTPUT_NAME atc.bin
        RUNTIME_OUTPUT_DIRECTORY atclib
        )

############ fwk_atc.bin ############
add_executable(fwk_atc.bin main.cc)

target_compile_options(fwk_atc.bin PRIVATE ${COMPILE_OPTIONS})

target_compile_definitions(fwk_atc.bin PRIVATE ${COMPILE_DEFINITIONS})

target_link_options(fwk_atc.bin PRIVATE
        -Wl,-Bsymbolic
        -Wl,-rpath-link,${ASCEND_ATC_DIR}/stub
        -Wl,-rpath-link,${ASCEND_RUNTIME_DIR}/stub
        )

target_link_libraries(fwk_atc.bin PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        atc_static
        )

set_target_properties(fwk_atc.bin PROPERTIES
        OUTPUT_NAME atc.bin
        RUNTIME_OUTPUT_DIRECTORY fwkacl
        )

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS atc_atc.bin OPTIONAL
        RUNTIME DESTINATION ${INSTALL_LIBRARY_DIR}/atclib
        )

install(TARGETS fwk_atc.bin OPTIONAL
        RUNTIME DESTINATION ${INSTALL_LIBRARY_DIR}/fwkacl
        )
