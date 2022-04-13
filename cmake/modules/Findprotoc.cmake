find_program(PROTOC_PROGRAM
            NAMES protoc
            PATHS ${CMAKE_INSTALL_PREFIX}/protoc/bin
            NO_DEFAULT_PATH
            )
mark_as_advanced(PROTOC_PROGRAM)
