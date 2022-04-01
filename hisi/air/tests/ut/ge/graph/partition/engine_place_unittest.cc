/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "compute_graph.h"
#include "graph/partition/engine_place.h"
#include "graph/debug/ge_attr_define.h"


namespace ge {

class UtestEnginePlace : public testing::Test {
  protected:
    void SetUp() {}

    void TearDown() {}
};

TEST_F(UtestEnginePlace, select_engine_by_attr_when_opdesc_empty) {
  std::string engine_name = "engine_name";
  std::string kernel_name = "kernel_name";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  AttrUtils::SetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, engine_name);
  AttrUtils::SetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, kernel_name);
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), SUCCESS);

  ASSERT_EQ(op_desc->GetOpEngineName(), engine_name);
  ASSERT_EQ(op_desc->GetOpKernelLibName(), kernel_name);
}

TEST_F(UtestEnginePlace, select_engine_by_when_opdesc_and_attr_empty) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), FAILED);

  ASSERT_TRUE(op_desc->GetOpEngineName().empty());
}

TEST_F(UtestEnginePlace, select_engine_when_opdesc_not_empty) {
  std::string engine_name = "engine_name";
  std::string kernel_name = "kernel_name";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  op_desc->SetOpEngineName(engine_name);
  op_desc->SetOpKernelLibName(kernel_name);
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), SUCCESS);

  std::string attr_engine_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, attr_engine_name);
  ASSERT_EQ(attr_engine_name, engine_name);
  std::string attr_kernel_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, attr_kernel_name);
  ASSERT_EQ(attr_kernel_name, kernel_name);
}

TEST_F(UtestEnginePlace, select_engine_when_opdesc_confilct_with_attr) {
  std::string op_engine_name = "op_engine_name";
  std::string op_kernel_name = "op_kernel_name";
  std::string attr_engine_name = "attr_engine_name";
  std::string attr_kernel_name = "attr_kernel_name";
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("default");
  auto op_desc = std::make_shared<OpDesc>("mock_op_name", "mock_op_type");
  op_desc->SetOpEngineName(op_engine_name);
  op_desc->SetOpKernelLibName(op_kernel_name);
  AttrUtils::SetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, attr_engine_name);
  AttrUtils::SetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, attr_kernel_name);
  auto node_ptr = graph->AddNode(op_desc);

  EnginePlacer engine_place(graph);
  bool is_check_support_success = true;
  ASSERT_EQ(engine_place.SelectEngine(node_ptr, is_check_support_success), SUCCESS);

  std::string fetched_attr_engine_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_ENGINE_NAME_FOR_LX, fetched_attr_engine_name);
  ASSERT_EQ(fetched_attr_engine_name, attr_engine_name);
  std::string fetched_attr_kernel_name;
  AttrUtils::GetStr(op_desc, ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, fetched_attr_kernel_name);
  ASSERT_EQ(fetched_attr_kernel_name, attr_kernel_name);

  ASSERT_EQ(op_desc->GetOpEngineName(), op_engine_name);
  ASSERT_EQ(op_desc->GetOpKernelLibName(), op_kernel_name);
}

} // namespace ge