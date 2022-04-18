set(OPENSOURCE_DIR ${TOP_DIR}/opensource)
set(COLLECT_DIR ${TOP_DIR}/collector)
########################### securec ############################
set(SECUREC_DIR ${OPENSOURCE_DIR}/securec)
file(GLOB_RECURSE SECUREC_SRC ${SECUREC_DIR}/src/*.c)

add_library(c_sec SHARED
    ${SECUREC_SRC}
)

target_include_directories(c_sec PRIVATE
    ${SECUREC_DIR}/include
)

target_compile_options(c_sec PRIVATE
    -fPIC
    -fstack-protector-all
    -fno-common
    -fno-strict-aliasing
    -Wfloat-equal
    -Wextra
)

target_link_options(c_sec PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -s
)

############################# protobuf_generate ##########################
function(protobuf_generate comp c_var h_var)
    if(NOT ARGN)
        message(SEND_ERROR "Error: protobuf_generate() called without any proto files")
        return()
    endif()

    set(${c_var})
    set(${h_var})
    set(_add_target FALSE)

    set(extra_option "")
    foreach(arg ${ARGN})
        if("${arg}" MATCHES "--proto_path")
            set(extra_option ${arg})
        endif()
    endforeach()


    foreach(file ${ARGN})
        if("${file}" MATCHES "--proto_path")
            continue()
        endif()

        if("${file}" STREQUAL "TARGET")
            set(_add_target TRUE)
            continue()
        endif()

        get_filename_component(abs_file ${file} ABSOLUTE)
        get_filename_component(file_name ${file} NAME_WE)
        get_filename_component(file_dir ${abs_file} PATH)
        get_filename_component(parent_subdir ${file_dir} NAME)

        if("${parent_subdir}" STREQUAL "proto")
            set(proto_output_path ${CMAKE_BINARY_DIR}/proto/${comp}/proto)
        else()
            set(proto_output_path ${CMAKE_BINARY_DIR}/proto/${comp}/proto/${parent_subdir})
        endif()
        list(APPEND ${c_var} "${proto_output_path}/${file_name}.pb.cc")
        list(APPEND ${h_var} "${proto_output_path}/${file_name}.pb.h")
        add_custom_command(
                OUTPUT "${proto_output_path}/${file_name}.pb.cc" "${proto_output_path}/${file_name}.pb.h"
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                COMMAND ${CMAKE_COMMAND} -E make_directory "${proto_output_path}"
                COMMAND ${CMAKE_COMMAND} -E echo "generate proto cpp_out ${comp} by ${abs_file}"
                COMMAND ${PROTOC_PROGRAM} -I${file_dir} ${extra_option} --cpp_out=${proto_output_path} ${abs_file}
                DEPENDS ${abs_file}
                COMMENT "Running C++ protocol buffer compiler on ${file}" VERBATIM )
    endforeach()

    if(_add_target)
        add_custom_target(
            ${comp} DEPENDS ${${c_var}} ${${h_var}}
        )
    endif()
    set_source_files_properties(${${c_var}} ${${h_var}} PROPERTIES GENERATED TRUE)
    set(${c_var} ${${c_var}} PARENT_SCOPE)
    set(${h_var} ${${h_var}} PARENT_SCOPE)

endfunction()

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
            COMMAND bash ${TOP_DIR}/llt/cmake/tools/llt_run_and_check.sh ${CMAKE_INSTALL_PREFIX}/${PACKAGE}_report $<TARGET_FILE:${${PACKAGE}_TARGET}> 
            ${${PACKAGE}_TASK_NUM} 1200 "${LLT_RUN_MOD}" ${${PACKAGE}_ENV_FILE}
            COMMAND echo "execute ${${PACKAGE}_TARGET} successfully"
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}
            DEPENDS ${${PACKAGE}_TARGET}
            WORKING_DIRECTORY ${TOP_DIR}
         )
    endif()

endfunction(run_llt_test)

######################################## alog ##################################
add_library(alog SHARED
    ${COLLECT_DIR}/dvvp/depend/stub/dlog_stub.cpp
)

target_include_directories(alog PRIVATE
    ${COLLECT_DIR}/dvvp/depend/inc/slog
)

target_compile_definitions(alog PRIVATE
    OS_TYPE_DEF=0
    LOG_CPP
    WRITE_TO_SYSLOG
)

target_compile_options(alog PRIVATE
    -Werror
    -Wfloat-equal -Wextra
    -O2
    -fPIC
    -fstack-protector-strong
    -fno-common
    -fsigned-char
    -fno-strict-aliasing
)

############################### error_manager #############################
add_library(error_manager SHARED
    ${COLLECT_DIR}/dvvp/depend/stub/error_manager/error_manager_stub.cpp
)

target_include_directories(error_manager PRIVATE
    ${COLLECT_DIR}/dvvp/depend/inc/metadef
)

target_compile_options(error_manager PRIVATE
    -std=c++11
    -Werror
    -fno-common
    -Wextra
    -Wfloat-equal
    -D_GLIBCXX_USE_CXX11_ABI=0
)

########################## profapi ##########################
add_library(profapi SHARED
    ${COLLECT_DIR}/dvvp/depend/stub/prof_api_stub.cpp
)

target_include_directories(profapi PRIVATE
    ${COLLECT_DIR}/dvvp/depend/inc/profapi
)

target_compile_options(profapi PRIVATE
    -std=c++11
    -fPIC
    -fstack-protector-all
    -fno-strict-aliasing
    -fno-common
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wfloat-equal
    -Wextra
)