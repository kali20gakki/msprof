set(ASCEND_PROTOBUF_STATIC_DIR ${CMAKE_INSTALL_PREFIX}/ascend_protobuf_static)

find_path(ASCEND_PROTOBUF_STATIC_INCLUDE
             PATHS ${ASCEND_PROTOBUF_STATIC_DIR}/include
             NO_DEFAULT_PATH
             CMAKE_FIND_ROOT_PATH_BOTH
             NAMES google/protobuf/api.pb.h)
mark_as_advanced(ASCEND_PROTOBUF_STATIC_INCLUDE)

find_library(ASCEND_PROTOBUF_STATIC_LIBRARY
            PATHS ${ASCEND_PROTOBUF_STATIC_DIR}/lib
            NO_DEFAULT_PATH
            CMAKE_FIND_ROOT_PATH_BOTH
            NAMES libascend_protobuf.a)
mark_as_advanced(ASCEND_PROTOBUF_STATIC_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ascend_protobuf_static
        REQUIRED_VARS ASCEND_PROTOBUF_STATIC_INCLUDE ASCEND_PROTOBUF_STATIC_LIBRARY
    )

    if(ascend_protobuf_static_FOUND)
        set(ASCEND_PROTOBUF_STATIC_INCLUDE_DIR ${ASCEND_PROTOBUF_STATIC_INCLUDE})
        set(ASCEND_PROTOBUF_STATIC_LIBRARY_DIR ${ASCEND_PROTOBUF_STATIC_DIR}/lib)
    
        if(NOT TARGET ascend_protobuf_static)
            add_library(ascend_protobuf_static STATIC IMPORTED)
            set_target_properties(ascend_protobuf_static PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${ASCEND_PROTOBUF_STATIC_INCLUDE}"
                IMPORTED_LOCATION             "${ASCEND_PROTOBUF_STATIC_LIBRARY}"
                )
        endif()
    endif()
