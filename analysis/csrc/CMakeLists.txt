cmake_minimum_required (VERSION 3.16)

project(msprof_analysis VERSION 0.1.0)

set(PROJECT_ROOT ${PROJECT_SOURCE_DIR}/../..)
set(CSRC ${PROJECT_SOURCE_DIR})
set(OPENSOURCE ${PROJECT_ROOT}/opensource)
set(COLLECTOR ${PROJECT_ROOT}/collector)

########################## securec ###################################
set(SECUREC_DIR ${PROJECT_ROOT}/platform/securec)
file(GLOB_RECURSE SECUREC_SRC ${SECUREC_DIR}/src/*.c)

add_library(c_sec SHARED
        ${SECUREC_SRC}
)

target_include_directories(c_sec PRIVATE
        ${SECUREC_DIR}/include
)

target_compile_options(c_sec PRIVATE
        -fPIC
        -fstack-protector-all
        -fno-common
        -fno-strict-aliasing
        -Wfloat-equal
        -Wextra
)

target_link_options(c_sec PRIVATE
        -Wl,-z,relro,-z,now,-z,noexecstack
        -s
)

set_target_properties(c_sec
        PROPERTIES
        OUTPUT_NAME c_sec
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/securec_shared
)

# 保持编译和运行python版本兼容性
add_definitions(-DASCEND_CI_LIMITED_PY37)

include_directories(${PROJECT_ROOT}/)

include_directories(${COLLECTOR}/inc/toolchain)
include_directories(${OPENSOURCE}/json/include)

include_directories(/usr/local/include/)
find_package(Python REQUIRED COMPONENTS Development)
include_directories(${Python_INCLUDE_DIRS})

# 严格控制so中源文件范围，所有源文件精细到最末端文件夹
aux_source_directory(${CSRC}/association SOURCES)
aux_source_directory(${CSRC}/association/cann SOURCES)
aux_source_directory(${CSRC}/external SOURCES)
aux_source_directory(${CSRC}/parser SOURCES)
aux_source_directory(${CSRC}/parser/adapter SOURCES)
aux_source_directory(${CSRC}/parser/environment SOURCES)
aux_source_directory(${CSRC}/parser/host/cann SOURCES)
aux_source_directory(${CSRC}/association/credential SOURCES)
aux_source_directory(${CSRC}/py_interface SOURCES)
aux_source_directory(${CSRC}/entities SOURCES)
aux_source_directory(${CSRC}/utils SOURCES)
aux_source_directory(${CSRC}/viewer SOURCES)
aux_source_directory(${CSRC}/viewer/database SOURCES)
aux_source_directory(${CSRC}/viewer/database/finals SOURCES)
aux_source_directory(${CSRC}/viewer/database/drafts SOURCES)
aux_source_directory(${CSRC}/worker SOURCES)
aux_source_directory(${CSRC}/backends SOURCES)
aux_source_directory(${CSRC}/dfx SOURCES)
aux_source_directory(${CSRC}/infrastructure/process SOURCES)
aux_source_directory(${CSRC}/infrastructure/data_inventory SOURCES)
aux_source_directory(${CSRC}/infrastructure/resource SOURCES)
aux_source_directory(${CSRC}/infrastructure/context SOURCES)
aux_source_directory(${CSRC}/domain/entities/hal SOURCES)
aux_source_directory(${CSRC}/domain/entities/metric SOURCES)
aux_source_directory(${CSRC}/domain/entities/pmu SOURCES)
aux_source_directory(${CSRC}/domain/services/constant SOURCES)
aux_source_directory(${CSRC}/domain/services/device_context SOURCES)
aux_source_directory(${CSRC}/domain/services/init SOURCES)
aux_source_directory(${CSRC}/domain/services/parser SOURCES)
aux_source_directory(${CSRC}/domain/services/parser/log SOURCES)
aux_source_directory(${CSRC}/domain/services/parser/pmu SOURCES)
aux_source_directory(${CSRC}/domain/services/parser/track SOURCES)
aux_source_directory(${CSRC}/domain/services/parser/parser_item SOURCES)
aux_source_directory(${CSRC}/domain/services/modeling SOURCES)
aux_source_directory(${CSRC}/domain/services/modeling/batch_id SOURCES)
aux_source_directory(${CSRC}/domain/services/modeling/step_trace SOURCES)
aux_source_directory(${CSRC}/domain/services/association/ SOURCES)
aux_source_directory(${CSRC}/domain/services/association/calculator/ SOURCES)
aux_source_directory(${CSRC}/domain/services/association/calculator/metric SOURCES)
aux_source_directory(${CSRC}/domain/services/association/calculator/hccl SOURCES)
aux_source_directory(${CSRC}/domain/services/association/calculator/metric/metric_calculator_group SOURCES)
aux_source_directory(${CSRC}/domain/services/persistence SOURCES)

add_library(msprof_analysis SHARED ${SOURCES})

target_include_directories(msprof_analysis PRIVATE
        ${SECUREC_DIR}/include
)

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
    c_sec
)
# 去掉lib前缀
set_target_properties(msprof_analysis PROPERTIES PREFIX  "")
# 指定安装路径
install(TARGETS msprof_analysis OPTIONAL
    LIBRARY DESTINATION lib64
)