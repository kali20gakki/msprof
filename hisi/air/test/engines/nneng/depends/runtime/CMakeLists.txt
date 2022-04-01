project(runtime_stub_fe)

set(FE_SRC_CPP
        src/runtime_stub.cpp)

# compile for fe_st
add_library(runtime_stub_fe SHARED
        ${FE_SRC_CPP})

if (BUILD_OPEN_PROJECT)
    target_include_directories(runtime_stub_fe PRIVATE
            ${AIR_ROOT_INC_DIR})
else()
    target_include_directories(runtime_stub_fe PRIVATE
            ${TOP_DIR}/air/third_party/inc)
endif()



# add it for slog compile at 12.16
target_compile_definitions(runtime_stub_fe PRIVATE
        LOG_CPP
        )

target_compile_options(runtime_stub_fe PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Dgoogle=ascend_private)

target_link_libraries(runtime_stub_fe PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        c_sec)

set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS runtime_stub_fe OPTIONAL
        EXPORT runtime_stub_fe-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR})
