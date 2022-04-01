if (HAVE_PROTOC_GRPC)
	return()
endif()

include(ExternalProject)
if ((${CMAKE_INSTALL_PREFIX} STREQUAL /usr/local) OR
    (${CMAKE_INSTALL_PREFIX} STREQUAL "C:/Program Files (x86)/ascend"))
    set(CMAKE_INSTALL_PREFIX ${AIR_CODE_DIR}/output CACHE STRING "path for install()" FORCE)
    MESSAGE(STATUS "no install prefix selected,default to ${CMAKE_INSTALL_PREFIX}.")
endif()

if (AIR_PB_PKG)
    set(REQ_URL "${AIR_PB_PKG}/libs/grpc/grpc-1.36.1.tar.gz")
    set(MD5 "")
elseif (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/grpc/repository/archive/v1.36.1.tar.gz")
    set(MD5 "")
else ()
    set(REQ_URL "https://github.com/grpc/grpc/archive/refs/tags/v1.36.1.tar.gz")
    set(MD5 "")
endif ()

set(GRPC_CXX_FLAGS "-Wl,-z,relro,-z,now,-z,noexecstack -D_FRTIFY_SOURCE=2 -O2 -fstack-protector-all -s -D_GLIBCXX_USE_CXX11_ABI=0")

ExternalProject_Add(protoc_grpc_build
	            URL ${REQ_URL}
                    CONFIGURE_COMMAND ${CMAKE_COMMAND}
                        # zlib
                        -DgRPC_ZLIB_PROVIDER=none
                        # cares
                        -DgRPC_CARES_PROVIDER=module
                        -DCARES_ROOT_DIR=${CMAKE_BINARY_DIR}/cares_build-prefix/src/cares_build
                        # re2
                        -DgRPC_RE2_PROVIDER=module
                        -DRE2_ROOT_DIR=${CMAKE_BINARY_DIR}/re2_build-prefix/src/re2_build
                        # absl
                        -DgRPC_ABSL_PROVIDER=module
                        -DABSL_ROOT_DIR=${CMAKE_BINARY_DIR}/abseil_build-prefix/src/abseil_build
                        # protobuf
                        -DgRPC_PROTOBUF_PROVIDER=module
                        -DPROTOBUF_ROOT_DIR=${CMAKE_BINARY_DIR}/protobuf_build-prefix/src/protobuf_build
                        # ssl
                        -DgRPC_SSL_PROVIDER=none
                        # grpc option
                        -DCMAKE_CXX_FLAGS=${GRPC_CXX_FLAGS}
                        -DCMAKE_BUILD_TYPE=Release
                        -DgRPC_BUILD_TESTS=OFF
                        -DCMAKE_INSTALL_LIBDIR=lib
                        -DgRPC_BUILD_CSHARP_EXT=OFF
                        <SOURCE_DIR>
                   BUILD_COMMAND $(MAKE) protoc grpc_cpp_plugin
                   INSTALL_COMMAND ""
                   EXCLUDE_FROM_ALL TRUE
)

add_dependencies(protoc_grpc_build re2_build cares_build abseil_build protobuf_build)

set(grpc_protoc_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build-build/third_party/protobuf/protoc)
set(grpc_grpc_cpp_plugin_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc_grpc_build-prefix/src/protoc_grpc_build-build/grpc_cpp_plugin)

function(protobuf_generate_grpc comp c_var h_var)
    if(NOT ARGN)
        MESSAGE(SEND_ERROR "Error:protobuf_generate_grpc() called without any proto files")
        return()
    endif()
    set(${c_var})
    set(${h_var})

    set(extra_option "")
    foreach(arg ${ARGN})
        if ("${arg}" MATCHES "--proto_path")
            set(extra_option ${arg})
        endif()
    endforeach()

    foreach(file ${ARGN})
        if ("${file}" MATCHES "--proto_path")
            continue()
        endif()
        get_filename_component(abs_file ${file} ABSOLUTE)
        get_filename_component(file_name ${file} NAME_WE)
        get_filename_component(file_dir ${abs_file} PATH)
        get_filename_component(parent_subdir ${file_dir} NAME)
        if("${parent_subdir}" STREQUAL "proto")
            set(proto_output_path ${CMAKE_BINARY_DIR}/proto_grpc/${comp}/proto)
            set(grpc_cpp_plugin_path ${CMAKE_BINARY_DIR}/grpc_host_build-prefix/src/grpc_host_build-build)
        else()
            set(proto_output_path ${CMAKE_BINARY_DIR}/proto_grpc/${comp}/proto_grpc/${parent_subdir})
        endif()
        list(APPEND ${c_var} "${proto_output_path}/${file_name}.pb.cc")
        list(APPEND ${c_var} "${proto_output_path}/${file_name}.grpc.pb.cc")
        list(APPEND ${h_var} "${proto_output_path}/${file_name}.pb.h")
        list(APPEND ${h_var} "${proto_output_path}/${file_name}.grpc.pb.h")

        add_custom_command(
                OUTPUT "${proto_output_path}/${file_name}.pb.cc" "${proto_output_path}/${file_name}.pb.h"
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} 
                COMMAND ${CMAKE_COMMAND} -E make_directory "${proto_output_path}"
		        COMMAND ${grpc_protoc_EXECUTABLE} -I${file_dir} ${extra_option} --cpp_out=${proto_output_path} ${abs_file}
                COMMAND ${grpc_protoc_EXECUTABLE} -I${file_dir} ${extra_option} --grpc_out=${proto_output_path} --plugin=protoc-gen-grpc=${grpc_grpc_cpp_plugin_EXECUTABLE} ${abs_file}
                DEPENDS protoc_grpc_build ${abs_file}
                COMMENT "Running C++ protocol buffer complier on ${file}" VERBATIM)
    endforeach()

    set_source_files_properties(${${c_var}} ${${h_var}} PROPERTIES GENERATED TRUE) 
    set(${c_var} ${${c_var}} PARENT_SCOPE)
    set(${h_var} ${${h_var}} PARENT_SCOPE)
    
endfunction()

set(HAVE_PROTOC_GRPC TRUE)
