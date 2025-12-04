set(PLATFORM_DIR ${TOP_DIR}/platform)
########################### securec ############################
set(SECUREC_DIR ${PLATFORM_DIR}/securec)
file(GLOB_RECURSE SECUREC_SRC ${SECUREC_DIR}/src/*.c)

add_library(c_sec_static STATIC
    ${SECUREC_SRC}
)

target_include_directories(c_sec_static PRIVATE
    ${SECUREC_DIR}/include
)

target_compile_options(c_sec_static PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

target_link_options(c_sec_static PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

##################################### run_llt_test ###############################
function(run_llt_test)
    cmake_parse_arguments(${PACKAGE} "" "TARGET;TASK_NUM;ENV_FILE" "" ${ARGN})
    if (( PACKAGE STREQUAL "ut") OR (PACKAGE STREQUAL "st"))
    add_custom_target(${PACKAGE}_${${PACKAGE}_TARGET} ALL DEPENDS ${CMAKE_INSTALL_PREFIX}/${${PACKAGE}_TARGET}.timestamp)
        if(NOT DEFINED ${PACKAGE}_TASK_NUM)
            set(${PACKAGE}_TASK_NUM 1)
        endif()

        if((NOT DEFINED LLT_RUN_MOD) OR (LLT_RUN_MOD STREQUAL ""))
            set(LLT_RUN_MOD single)
        endif()

    if((NOT DEFINED ${PACKAGE}_ENV_FILE) OR (${PACKAGE}_ENV_FILE STREQUAL ""))
        set(${PACKAGE}_ENV_FILE \"\")
    endif()
        add_custom_command(
            OUTPUT ${CMAKE_INSTALL_PREFIX}/${${PACKAGE}_TARGET}.timestamp
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/${PACKAGE}_report
            COMMAND echo "execute ${${PACKAGE}_TARGET} begin:"
            COMMAND bash ${TOP_DIR}/test/cmake/tools/llt_run_and_check.sh ${CMAKE_INSTALL_PREFIX}/${PACKAGE}_report $<TARGET_FILE:${${PACKAGE}_TARGET}> 
            ${${PACKAGE}_TASK_NUM} 1200 "${LLT_RUN_MOD}" ${${PACKAGE}_ENV_FILE}
            COMMAND echo "execute ${${PACKAGE}_TARGET} successfully"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}
            DEPENDS ${${PACKAGE}_TARGET}
            WORKING_DIRECTORY ${TOP_DIR}
         )
    endif()

endfunction(run_llt_test)
