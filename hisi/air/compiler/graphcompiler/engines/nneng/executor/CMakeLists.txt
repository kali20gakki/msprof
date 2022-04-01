project(fe_executor)

file(GLOB FE_EXECUTOR_SRC_CPP_1 RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "ffts_plus_update/*.cc")

#compiler for host
add_library(fe_executor SHARED
        ${FE_EXECUTOR_SRC_CPP_1})

target_include_directories(fe_executor PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/graph/
        ${TOP_DIR}/graphengine/inc
        #### yellow zone ####
        $<$<NOT:$<BOOL:${ENABLE_OPEN_SRC}>>:${TOP_DIR}/abl>
        #### blue zone ####
        $<$<BOOL:${ENABLE_OPEN_SRC}>:${TOP_DIR}/third_party/inc/qos>
        )

target_compile_options(fe_executor PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -O2
        -Wfloat-equal
        -fexceptions
        -fno-common
        -fno-strict-aliasing
        -Dgoogle=ascend_private)

target_compile_definitions(fe_executor PRIVATE
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(fe_executor PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        -Wl,--no-as-needed
        c_sec
        slog
        graph
        error_manager
       #qos_manager
        json
        -ldl)

set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS fe_executor OPTIONAL
        EXPORT fe_executor-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )
