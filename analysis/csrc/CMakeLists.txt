cmake_minimum_required (VERSION 3.16)

project(msprof_analysis VERSION 0.1.0)

set(PROJECT_ROOT ${PROJECT_SOURCE_DIR}/../..)
set(CSRC ${PROJECT_SOURCE_DIR})
set(OPENSOURCE ${PROJECT_ROOT}/opensource)
set(COLLECTOR ${PROJECT_ROOT}/collector)

# 保持编译和运行python版本兼容性
add_definitions(-DASCEND_CI_LIMITED_PY37)

include_directories(${CSRC}/association)
include_directories(${CSRC}/association/cann)
include_directories(${CSRC}/external)
include_directories(${CSRC}/parser)
include_directories(${CSRC}/parser/adapter)
include_directories(${CSRC}/parser/environment)
include_directories(${CSRC}/parser/host/cann)
include_directories(${CSRC}/py_interface)
include_directories(${CSRC}/entities)
include_directories(${CSRC}/utils)
include_directories(${CSRC}/viewer)
include_directories(${CSRC}/viewer/database)
include_directories(${CSRC}/worker)
include_directories(${CSRC}/dfx)
include_directories(${COLLECTOR}/inc/toolchain)
include_directories(${OPENSOURCE}/json/include)

include_directories(/usr/local/include/)
include_directories(/opt/buildtools/python-3.9.11/include/python3.9)

aux_source_directory(${CSRC}/association SOURCES)
aux_source_directory(${CSRC}/association/cann SOURCES)
aux_source_directory(${CSRC}/external SOURCES)
aux_source_directory(${CSRC}/parser SOURCES)
aux_source_directory(${CSRC}/parser/adapter SOURCES)
aux_source_directory(${CSRC}/parser/environment SOURCES)
aux_source_directory(${CSRC}/parser/host/cann SOURCES)
aux_source_directory(${CSRC}/py_interface SOURCES)
aux_source_directory(${CSRC}/entities SOURCES)
aux_source_directory(${CSRC}/utils SOURCES)
aux_source_directory(${CSRC}/viewer SOURCES)
aux_source_directory(${CSRC}/viewer/database SOURCES)
aux_source_directory(${CSRC}/worker SOURCES)
aux_source_directory(${CSRC}/backends SOURCES)
aux_source_directory(${CSRC}/dfx SOURCES)

add_library(msprof_analysis SHARED ${SOURCES})

target_compile_options(msprof_analysis PRIVATE
    -std=c++11
    -fPIC
    -fstack-protector-all
    -fno-strict-aliasing
    -fno-common
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -Wfloat-equal
    -Wextra
    -D_GLIBCXX_USE_CXX11_ABI=0
    -D_FORTIFY_SOURCE=2
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-O2>
    -fstack-check
    -ftrapv
)

target_link_options(msprof_analysis PRIVATE
    -Wl,-z,relro,-z,now,-z,noexecstack
    -Wl,-Bsymbolic
    -Wl,--exclude-libs=ALL
    $<$<NOT:$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>>:-s>
)

target_link_libraries(msprof_analysis PRIVATE
    -lsqlite3
    -lpthread
)
# 去掉lib前缀
set_target_properties(msprof_analysis PROPERTIES PREFIX  "")
# 指定安装路径
install(TARGETS msprof_analysis OPTIONAL
    LIBRARY DESTINATION lib64
)