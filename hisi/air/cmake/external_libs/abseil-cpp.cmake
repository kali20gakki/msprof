if (HAVE_ABSEIL)
    return()
endif()

include(ExternalProject)

if (AIR_PB_PKG)
    set(REQ_URL "${AIR_PB_PKG}/libs/abseil-cpp/abseil-cpp-20200923.3.tar.gz")
    set(MD5 "")
elseif (ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/abseil-cpp/repository/archive/20200923.3.tar.gz")
    set(MD5 "")
else ()
    set(REQ_URL "https://github.com/abseil/abseil-cpp/archive/refs/tags/20200923.3.tar.gz")
    set(MD5 "")
endif ()

ExternalProject_Add(abseil_build
    URL ${REQ_URL}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    EXCLUDE_FROM_ALL TRUE
    DWONLOAD_NO_PROGRESS TRUE
)

set(HAVE_ABSEIL TRUE)
