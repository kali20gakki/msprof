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

##################################### depend #########################################
# error_manager
add_library(error_manager SHARED
    ${TOP_DIR}/collector/dvvp/depend/stub/error_manager/error_manager_stub.cpp
)

target_include_directories(error_manager PRIVATE
    ${TOP_DIR}/collector/dvvp/depend/inc/metadef
)

target_compile_options(error_manager PRIVATE
    -std=c++11
    -Werror
    -fno-common
    -Wextra
    -Wfloat-equal
    -D_GLIBCXX_USE_CXX11_ABI=0
)

# profapi
add_library(profapi SHARED
    ${TOP_DIR}/collector/dvvp/depend/stub/prof_api_stub.cpp
)

target_include_directories(profapi PRIVATE
    ${TOP_DIR}/collector/dvvp/depend/inc/profapi
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

######################################################################################

add_subdirectory(dvvp)