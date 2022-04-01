project(te_fusion_stub)

set(TE_FUSION_SRC_CPP
        src/te_fusion_api.cc)

# compile for fe_st
add_library(te_fusion_stub SHARED
        ${TE_FUSION_SRC_CPP})

if (BUILD_OPEN_PROJECT)
    target_include_directories(te_fusion_stub PRIVATE
            ${AIR_ROOT_INC_DIR}
            ${METADEF_DIR}/inc
            ${METADEF_DIR}/inc/external)
else()
    target_include_directories(te_fusion_stub PRIVATE
            ${TOP_DIR}/air/third_party/inc)
endif()



# add it for slog compile at 12.16
target_compile_definitions(te_fusion_stub PRIVATE
        LOG_CPP
        )

target_compile_options(te_fusion_stub PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=c++11>
        -Dgoogle=ascend_private)

target_link_libraries(te_fusion_stub PRIVATE
        $<BUILD_INTERFACE:intf_pub>
        c_sec)

set(INSTALL_BASE_DIR "")
set(INSTALL_INCLUDE_DIR include)
set(INSTALL_LIBRARY_DIR lib)
set(INSTALL_CONFIG_DIR  lib/cmake)

install(TARGETS te_fusion_stub OPTIONAL
        EXPORT te_fusion_stub-targets
        LIBRARY DESTINATION ${INSTALL_LIBRARY_DIR})
