set(SRC_LIST
    "dnnengines.cc"
    "engine_manage.cc"
)

############ libengine.so ############
add_library(engine SHARED ${SRC_LIST})

add_dependencies(engine
    graphengine_protos
)

target_compile_options(engine PRIVATE
    -Werror
    -Wextra
    -Wfloat-equal
    -fno-common
    -fvisibility=hidden
)

target_compile_definitions(engine PRIVATE
    REUSE_MEMORY=1
    PROTOBUF_INLINE_NOT_IN_HEADERS=0
    FUNC_VISIBILITY
)

target_include_directories(engine PRIVATE
    ${AIR_CODE_DIR}/base
    ${AIR_CODE_DIR}/compiler
    ${AIR_CODE_DIR}/inc
    ${AIR_CODE_DIR}/inc/framework
    ${AIR_CODE_DIR}/inc/external
    ${METADEF_DIR}/inc
    ${METADEF_DIR}/inc/external
    #### yellow zone ####
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${GE_DEPEND_DIR}/inc>
    #### blue zone ####
    $<$<BOOL:${ENABLE_OPEN_SRC}>:${AIR_CODE_DIR}/third_party/inc>
)

target_link_options(engine PRIVATE
    -Wl,-Bsymbolic
)

target_link_libraries(engine PRIVATE
    $<BUILD_INTERFACE:intf_pub>
    $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:$<BUILD_INTERFACE:slog_headers>>
    -Wl,--no-as-needed
    c_sec
    slog
    -Wl,--as-needed
    -lrt
    -ldl
)

############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS engine OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
)
