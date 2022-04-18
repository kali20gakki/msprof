set(GTEST_DIR ${CMAKE_INSTALL_PREFIX}/gtest)

find_path(GTEST_INCLUDE
    PATHS ${GTEST_DIR}/include
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES gtest/gtest.h)
mark_as_advanced(GTEST_INCLUDE)

find_path(GMOCK_INCLUDE
    PATHS ${GTEST_DIR}/include
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES gmock/gmock.h)
mark_as_advanced(GMOCK_INCLUDE)


find_library(GTEST_STATIC_LIBRARY
    PATHS ${GTEST_DIR}/lib
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES libgtest.a)
mark_as_advanced(GTEST_STATIC_LIBRARY)

find_library(GTEST_MAIN_STATIC_LIBRARY
    PATHS ${GTEST_DIR}/lib
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES libgtest_main.a)
mark_as_advanced(GTEST_MAIN_STATIC_LIBRARY)

find_library(GMOCK_STATIC_LIBRARY
    PATHS ${GTEST_DIR}/lib
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES libgmock.a)
mark_as_advanced(GMOCK_STATIC_LIBRARY)

find_library(GMOCK_MAIN_STATIC_LIBRARY
    PATHS ${GTEST_DIR}/lib
    NO_DEFAULT_PATH
    CMAKE_FIND_ROOT_PATH_BOTH
    NAMES libgmock_main.a)
mark_as_advanced(GMOCK_MAIN_STATIC_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gtest
        REQUIRED_VARS GTEST_INCLUDE GMOCK_INCLUDE GTEST_STATIC_LIBRARY GTEST_MAIN_STATIC_LIBRARY GMOCK_STATIC_LIBRARY GMOCK_MAIN_STATIC_LIBRARY
    )

	if(gtest_FOUND)
    set(GTEST_INCLUDE_DIR ${GTEST_INCLUDE})
    get_filename_component(GTEST_LIBRARY_DIR ${GTEST_STATIC_LIBRARY} DIRECTORY)

    if(NOT TARGET gtest_static)
        add_library(gtest_static STATIC IMPORTED)
        set_target_properties(gtest_static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
            IMPORTED_LOCATION "${GTEST_STATIC_LIBRARY}"
            )
    endif()

    if(NOT TARGET gtest_main_static)
        add_library(gtest_main_static STATIC IMPORTED)
        set_target_properties(gtest_main_static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
            IMPORTED_LOCATION "${GTEST_MAIN_STATIC_LIBRARY}"
            )
    endif()

    if(NOT TARGET gmock_static)
        add_library(gmock_static STATIC IMPORTED)
        set_target_properties(gmock_static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
            IMPORTED_LOCATION "${GMOCK_STATIC_LIBRARY}"
            )
    endif()

    if(NOT TARGET gmock_main_static)
        add_library(gmock_main_static STATIC IMPORTED)
        set_target_properties(gmock_main_static PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
            IMPORTED_LOCATION "${GMOCK_MAIN_STATIC_LIBRARY}"
            )
    endif()

endif()

