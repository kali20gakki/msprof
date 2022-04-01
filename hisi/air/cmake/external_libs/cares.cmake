if (HAVE_CARES)
    return()
endif()

include(ExternalProject)

if (AIR_PB_PKG)
    set(REQ_URL "${AIR_PB_PKG}/libs/cares/c-ares-cares-1_16_1.tar.gz")
    set(MD5 "")
elseif (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/c-ares/repository/archive/cares-1_16_1.tar.gz")
    set(MD5 "")
else ()
    set(REQ_URL "https://github.com/c-ares/c-ares/archive/refs/tags/cares-1_16_1.tar.gz")
    set(MD5 "")
endif ()

ExternalProject_Add(cares_build
                    URL ${REQ_URL}
		    CONFIGURE_COMMAND ""
     		    BUILD_COMMAND ""
                    INSTALL_COMMAND ""
		    EXCLUDE_FROM_ALL TRUE
		    DWONLOAD_NO_PROGRESS TRUE
)

set(HAVE_CARES TRUE)
