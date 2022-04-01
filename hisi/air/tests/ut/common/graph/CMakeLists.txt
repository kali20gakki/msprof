# Copyright 2019-2020 Huawei Technologies Co., Ltd
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ============================================================================

project(ut_libgraph)

set(CMAKE_CXX_STANDARD 11)

# include directories
include_directories(${CMAKE_CURRENT_LIST_DIR})

include_directories(${METADEF_DIR})
include_directories(${METADEF_DIR}/graph)
include_directories(${METADEF_DIR}/inc)
include_directories(${METADEF_DIR}/inc/graph)
include_directories(${METADEF_DIR}/inc/external)
include_directories(${METADEF_DIR}/inc/external/graph)
include_directories(${METADEF_DIR}/third_party/transformer/inc)
include_directories(${AIR_CODE_DIR}/inc)
include_directories(${AIR_CODE_DIR}/inc/external)
include_directories(${AIR_CODE_DIR}/third_party/inc)

include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_BINARY_DIR}/proto/ge)

set(UT_FILES
    "testcase/ge_graph/ge_anchor_utils_unittest.cc"
    "testcase/ge_graph/ge_def_type_unittest.cc"
    "testcase/ge_graph/ge_graph_anchor_unittest.cc"
    "testcase/ge_graph/ge_model_serialize_unittest.cc"
    "testcase/ge_graph/ge_node_unittest.cc"
    "testcase/ge_graph/ge_opdesc_unittest.cc"
    "testcase/ge_graph/ge_tensor_unittest.cc"
    "testcase/ge_graph/graph_builder_utils.cc"
    "testcase/ge_graph/ge_reg_unittest.cc"
    "testcase/ge_graph/ge_graph_dsl_check_test.cc"
    "testcase/ge_graph/ge_format_refiner_unittest.cc"
    "testcase/ge_graph/ge_opsproto_manager_unittest.cc"
    "testcase/ge_graph/ge_operator_unittest.cc"
    "testcase/ge_graph/ge_model_unittest.cc"
)

add_executable(ut_libgraph ${UT_FILES} ${AIR_CODE_DIR}/base/common/types.cc)

target_compile_options(ut_libgraph PRIVATE
    -g --coverage -fprofile-arcs -ftest-coverage
)

target_compile_definitions(ut_libgraph PRIVATE
    google=ascend_private
)

set_target_properties(ut_libgraph PROPERTIES CXX_STANDARD 17)

target_link_libraries(ut_libgraph
    $<BUILD_INTERFACE:intf_pub>
    gtest
    gtest_main
    ge_graph_dsl
)
