project(opskernel)

file(GLOB OPSKERNEL_SRC_CPP_1 RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        "ops_store/*.cc")

#compiler for host
add_library(opskernel SHARED
        ${OPSKERNEL_SRC_CPP_1})

target_include_directories(opskernel PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/graph/
        ${TOP_DIR}/graphengine/inc)

target_compile_options(opskernel PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Wfloat-equal
        -Werror
        -fexceptions
        -fno-common
        -fno-strict-aliasing)

target_compile_definitions(opskernel PRIVATE
        google=ascend_private
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(opskernel PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        -O2
        c_sec
        slog
        graph
        error_manager
        aicore_utils
        -Wl,--no-as-needed
        json
        -ldl)

#compiler for atc host
add_library(atc_opskernel SHARED
        ${OPSKERNEL_SRC_CPP_1})

set_target_properties(atc_opskernel
        PROPERTIES
        OUTPUT_NAME opskernel
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/atclib)

target_include_directories(atc_opskernel PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${NN_ENG_DIR}/inc/
        ${AIR_ROOT_INC_DIR}
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc/external/
        ${METADEF_DIR}/inc/framework/
        ${METADEF_DIR}/inc/register/
        ${METADEF_DIR}/graph/
        ${TOP_DIR}/graphengine/inc)

target_compile_options(atc_opskernel PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Werror
        -Wfloat-equal
        -fexceptions
        -fno-common
        -fno-strict-aliasing)

target_compile_definitions(atc_opskernel PRIVATE
        google=ascend_private
        $<$<STREQUAL:${BUILD_OPEN_PROJECT},True>:ONLY_COMPILE_OPEN_SRC>
        )

target_link_libraries(atc_opskernel PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:msprof_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:slog_headers>>
        $<$<NOT:$<BOOL:${BUILD_OPEN_PROJECT}>>:$<BUILD_INTERFACE:runtime_headers>>
        -O2
        c_sec
        slog
        graph
        error_manager
        atc_aicore_utils
        -Wl,--no-as-needed
        json
        -ldl)

set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS opskernel OPTIONAL
        EXPORT opskernel-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}
        )

install(TARGETS atc_opskernel OPTIONAL
        EXPORT atc_opskernel-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}/atclib
        )
