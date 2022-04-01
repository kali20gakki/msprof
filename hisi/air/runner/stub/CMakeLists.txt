project (stubs)

if (NOT ENABLE_D AND NOT ENABLE_ACL AND NOT ENABLE_MS_TESTCASES)
##################################################################
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/stub_ge_ir_build.cc
           ${CMAKE_CURRENT_BINARY_DIR}/stub_ge_api.cc
    COMMAND echo "Generating stub files."
            && ${HI_PYTHON} ${CMAKE_CURRENT_LIST_DIR}/gen_stubapi.py ${AIR_CODE_DIR}/inc/external ${CMAKE_CURRENT_BINARY_DIR}
            && mv ge_ir_build.cc stub_ge_ir_build.cc
            && mv ge_api.cc stub_ge_api.cc
            &&  echo "Generating stub files end."
    #WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    #DEPENDS stub/gen_stubapi.py ${TOP_DIR}/inc/external ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(ge_stub
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/stub_ge_ir_build.cc
            ${CMAKE_CURRENT_BINARY_DIR}/stub_ge_api.cc
)

##################################################################
############ stub/libge_compiler.so ############
add_library(atc_stub_ge_compiler SHARED
    stub_ge_ir_build.cc
)

add_dependencies(atc_stub_ge_compiler ge_stub)

target_compile_options(atc_stub_ge_compiler PRIVATE
    -fno-common
)

target_link_libraries(atc_stub_ge_compiler PRIVATE
    $<BUILD_INTERFACE:intf_pub>
)

set_target_properties(atc_stub_ge_compiler PROPERTIES
    OUTPUT_NAME ge_compiler
    LIBRARY_OUTPUT_DIRECTORY atc_stub
)

target_include_directories(atc_stub_ge_compiler PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${AIR_CODE_DIR}/inc/external
    ${METADEF_DIR}/inc/external
)

############ stub/libge_runner.so ############
add_library(fwk_stub_ge_runner SHARED
    stub_ge_api.cc
    stub_ge_ir_build.cc
)

add_dependencies(fwk_stub_ge_runner ge_stub)

target_compile_options(fwk_stub_ge_runner PRIVATE
    -fno-common
)

target_link_libraries(fwk_stub_ge_runner PRIVATE
    $<BUILD_INTERFACE:intf_pub>
)

set_target_properties(fwk_stub_ge_runner PROPERTIES
    OUTPUT_NAME ge_runner
    LIBRARY_OUTPUT_DIRECTORY fwk_stub
)

target_include_directories(fwk_stub_ge_runner PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${AIR_CODE_DIR}/inc/external
    ${METADEF_DIR}/inc/external
)

###############################################################
############ install ############
set(INSTALL_BASE_DIR "")
set(INSTALL_LIBRARY_DIR lib)

install(TARGETS atc_stub_ge_compiler fwk_stub_ge_runner OPTIONAL
    LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR}/stub
)

endif()
