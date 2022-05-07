set(ASCEND_PROTOBUF_SHARED_DIR ${CMAKE_INSTALL_PREFIX}/ascend_protobuf_shared)

find_path(ASCEND_PROTOBUF_SHARED_INCLUDE
             PATHS ${ASCEND_PROTOBUF_SHARED_DIR}/include
             NO_DEFAULT_PATH
             CMAKE_FIND_ROOT_PATH_BOTH
             NAMES google/protobuf/api.pb.h)
mark_as_advanced(ASCEND_PROTOBUF_SHARED_INCLUDE)

find_library(ASCEND_PROTOBUF_SHARED_LIBRARY
            PATHS ${ASCEND_PROTOBUF_SHARED_DIR}/lib
            NO_DEFAULT_PATH
            CMAKE_FIND_ROOT_PATH_BOTH
            NAMES libascend_protobuf.so libprotobufd.lib libprotobuf.lib)
mark_as_advanced(ASCEND_PROTOBUF_SHARED_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ascend_protobuf_shared
        REQUIRED_VARS ASCEND_PROTOBUF_SHARED_INCLUDE ASCEND_PROTOBUF_SHARED_LIBRARY
    )

    if(ascend_protobuf_shared_FOUND)
        set(ASCEND_PROTOBUF_SHARED_INCLUDE_DIR ${ASCEND_PROTOBUF_SHARED_INCLUDE})
        set(ASCEND_PROTOBUF_SHARED_LIBRARY_DIR ${ASCEND_PROTOBUF_SHARED_DIR}/lib)
    
        if(NOT TARGET ascend_protobuf_shared)
            add_library(ascend_protobuf_shared SHARED IMPORTED)
            set_target_properties(ascend_protobuf_shared PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${ASCEND_PROTOBUF_SHARED_INCLUDE}"
                IMPORTED_LOCATION             "${ASCEND_PROTOBUF_SHARED_LIBRARY}"
                )
        endif()
    endif()
