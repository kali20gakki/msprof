if (BUILD_OPEN_PROJECT)
    SET(FE_ROOT_PATH ${AIR_CODE_DIR}/compiler/graphcompiler/engines/nneng)
    SET(FWK_TEST_PATH ${AIR_CODE_DIR}/tests)
    SET(THIRD_PARTY_DIR ${AIR_CODE_DIR}/third_party)
    SET(AIR_DIR ${AIR_CODE_DIR})
else()
    SET(FE_ROOT_PATH ${TOP_DIR}/air/compiler/graphcompiler/engines/nneng)
    SET(METADEF_DIR ${TOP_DIR}/metadef)
    SET(FWK_TEST_PATH ${TOP_DIR}/air/tests)
    SET(THIRD_PARTY_DIR ${TOP_DIR}/air/third_party)
    SET(AIR_DIR ${TOP_DIR}/air)
endif()


#-------------------------ge_graph_dsl------------------------------
add_subdirectory(${FWK_TEST_PATH}/framework/easy_graph easy_graph)
add_subdirectory(${FWK_TEST_PATH}/framework/ge_graph_dsl/include ge_graph_dsl_inc)
file(GLOB_RECURSE SOURCES
        ${AIR_DIR}/base/common/types.cc
        ${FWK_TEST_PATH}/framework/ge_graph_dsl/src/assert/*.cc
        ${FWK_TEST_PATH}/framework/ge_graph_dsl/src/op_desc/*.cc
        ${FWK_TEST_PATH}/framework/ge_graph_dsl/src/vistor/*.cc)

add_library(ge_graph_dsl STATIC ${SOURCES})

target_link_libraries(ge_graph_dsl PUBLIC
        $<BUILD_INTERFACE:intf_pub>)

set_target_properties(ge_graph_dsl PROPERTIES CXX_STANDARD 17)
target_link_libraries(ge_graph_dsl PUBLIC ge_graph_dsl_inc easy_graph graph)
target_include_directories(ge_graph_dsl PRIVATE
        ${FWK_TEST_PATH}/framework/ge_graph_dsl/include
        ${FWK_TEST_PATH}/framework/easy_graph/include
        ${FWK_TEST_PATH}/tests/framework/easy_graph/include
        ${AIR_DIR}/inc/external
        ${AIR_DIR}/inc
        ${AIR_DIR}/inc/framework
        ${AIR_DIR}/inc/framework/common

        ${METADEF_DIR}
        ${METADEF_DIR}/graph
        ${METADEF_DIR}/inc/external
        ${METADEF_DIR}/inc/external/graph
        ${METADEF_DIR}/inc/graph
        ${METADEF_DIR}/inc/common
        ${METADEF_DIR}/inc
        ${METADEF_DIR}/register
        ${METADEF_DIR}/third_party/transformer
        ${METADEF_DIR}/third_party/transformer/inc
        ${METADEF_DIR}/third_party/transformer
        ${THIRD_PARTY_DIR}/inc
        )
#-------------------------------------------------------------------