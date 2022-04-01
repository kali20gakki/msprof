if (HAVE_RE2)
    return()
endif()

include(ExternalProject)

if (AIR_PB_PKG)
    set(REQ_URL "${AIR_PB_PKG}/libs/re2/re2-2019-12-01.tar.gz")
    set(MD5 "")
elseif (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/re2/repository/archive/2019-12-01.tar.gz")
    set(MD5 "")
else ()
    set(REQ_URL "https://github.com/google/re2/archive/refs/tags/2019-12-01.tar.gz")
    set(MD5 "")
endif ()

ExternalProject_Add(re2_build
    URL ${REQ_URL}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
    DWONLOAD_NO_PROGRESS TRUE
)

set(HAVE_RE2 TRUE)
