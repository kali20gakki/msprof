if (HAVE_ZLIB)
    return()
endif()

include(ExternalProject)

if (AIR_PB_PKG)
    set(REQ_URL "${AIR_PB_PKG}/libs/zlib/zlib-1.2.11.tar.gz")
    set(MD5 "")
elseif (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/zlib/repository/archive/v1.2.11.tar.gz")
    set(MD5 "")
else ()
    set(REQ_URL "https://github.com/madler/zlib/archive/refs/tags/v1.2.11.tar.gz")
    set(MD5 "")
endif ()

ExternalProject_Add(zlib_build
                    URL ${REQ_URL}
		    CONFIGURE_COMMAND ""
     		    BUILD_COMMAND ""
                    INSTALL_COMMAND ""
		    EXCLUDE_FROM_ALL TRUE
		    DWONLOAD_NO_PROGRESS TRUE
)

set(HAVE_ZLIB TRUE)
