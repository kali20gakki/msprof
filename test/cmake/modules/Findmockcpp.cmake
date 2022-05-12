set(MOCKCPP_DIR ${CMAKE_INSTALL_PREFIX}/mockcpp)

find_path(MOCKCPP_INCLUDE
    PATHS ${MOCKCPP_DIR}/include
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES mockcpp/mockcpp.hpp)
mark_as_advanced(MOCKCPP_INCLUDE)

find_library(MOCKCPP_STATIC_LIBRARY
    PATHS ${MOCKCPP_DIR}/lib
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES libmockcpp.a)
mark_as_advanced(MOCKCPP_STATIC_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mockcpp
        REQUIRED_VARS MOCKCPP_INCLUDE MOCKCPP_STATIC_LIBRARY
    )

    if(mockcpp_FOUND)
    set(MOCKCPP_INCLUDE_DIR ${MOCKCPP_INCLUDE})
    get_filename_component(MOCKCPP_LIBRARY_DIR ${MOCKCPP_STATIC_LIBRARY} DIRECTORY)
    endif()