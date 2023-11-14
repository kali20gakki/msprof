cmake_minimum_required (VERSION 3.16)

project(msprof_analysis VERSION 0.1.0)

set(PROJECT_ROOT ${PROJECT_SOURCE_DIR}/../..)
set(CSRC ${PROJECT_SOURCE_DIR})
set(OPENSOURCE ${PROJECT_ROOT}/opensource)

add_compile_options("-std=c++17")
# 保持编译和运行python版本兼容性
add_definitions(-DASCEND_CI_LIMITED_PY37)
if (CMAKE_BUILD_TYPE STREQUAL DEBUG)
    add_definitions(-DDEBUG)
endif()

# 安全规范编译命令
add_compile_options("-fPIE")
add_compile_options("-fPIC")
add_compile_options("-fstack-protector-all")
add_compile_options("-Wall")
add_compile_options("-D_FORTIFY_SOURCE=2")
add_compile_options("-fno-strict-aliasing")
add_compile_options("-O2")

add_link_options("-Wl,-z,now")
add_link_options("-Wl,-z,relro")
add_link_options("-Wl,-z,noexecstack")
add_link_options("-pie")
add_link_options("-s") #strip

include_directories(${CSRC}/association)
include_directories(${CSRC}/association/cann)
include_directories(${CSRC}/external)
include_directories(${CSRC}/parser)
include_directories(${CSRC}/parser/host/cann)
include_directories(${CSRC}/py_interface)
include_directories(${CSRC}/entities)
include_directories(${CSRC}/utils)
include_directories(${CSRC}/viewer)
include_directories(${CSRC}/viewer/database)
include_directories(${CSRC}/worker)
include_directories(${CSRC}/dfx)
include_directories(${OPENSOURCE}/json/include)

include_directories(/usr/local/include/)
include_directories(/usr/local/python3.9.2/include/python3.9)

aux_source_directory(${CSRC}/association SOURCES)
aux_source_directory(${CSRC}/association/cann SOURCES)
aux_source_directory(${CSRC}/external SOURCES)
aux_source_directory(${CSRC}/parser SOURCES)
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
# 去掉lib前缀
set_target_properties(msprof_analysis PROPERTIES PREFIX  "")