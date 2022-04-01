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
#define protected public
#define private public
#include "graph/load/model_manager/model_utils.h"
#include "graph/manager/graph_var_manager.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#include "ut/ge/ffts_plus_proto_tools.h"
#include "framework/common/types.h"

using namespace std;

namespace ge {
class UtestModelUtils : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

// test ModelUtils::GetVarAddr
TEST_F(UtestModelUtils, get_var_addr_hbm) {
  uint8_t test = 2;
  uint8_t *pf = &test;
  RuntimeParam runtime_param;
  runtime_param.session_id = 0;
  runtime_param.logic_var_base = 0;
  runtime_param.var_base = reinterpret_cast<uintptr_t>(pf);
  runtime_param.var_size = 16;

  int64_t offset = 8;
  EXPECT_EQ(VarManager::Instance(runtime_param.session_id)->Init(0, 0, 0, 0), SUCCESS);
  EXPECT_NE(VarManager::Instance(runtime_param.session_id)->var_resource_, nullptr);
  VarManager::Instance(runtime_param.session_id)->var_resource_->var_offset_map_[offset] = RT_MEMORY_HBM;
  std::shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("test", "test");
  void *var_addr = nullptr;
  EXPECT_EQ(ModelUtils::GetVarAddr(runtime_param, op_desc, offset, 0, var_addr), SUCCESS);
  EXPECT_EQ(runtime_param.var_base + offset - runtime_param.logic_var_base, reinterpret_cast<uintptr_t>(var_addr));
  VarManager::Instance(runtime_param.session_id)->Destory();
}

TEST_F(UtestModelUtils, get_var_addr_rdma_hbm) {
  uint8_t test = 2;
  uint8_t *pf = &test;
  RuntimeParam runtime_param;
  runtime_param.session_id = 0;
  runtime_param.logic_var_base = 0;
  runtime_param.var_base = reinterpret_cast<uintptr_t>(pf);

  int64_t offset = 8;
  EXPECT_EQ(VarManager::Instance(runtime_param.session_id)->Init(0, 0, 0, 0), SUCCESS);
  EXPECT_NE(VarManager::Instance(runtime_param.session_id)->var_resource_, nullptr);
  VarManager::Instance(runtime_param.session_id)->var_resource_->var_offset_map_[offset] = RT_MEMORY_RDMA_HBM;
  std::shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("test", "test");
  void *var_addr = nullptr;
  EXPECT_EQ(ModelUtils::GetVarAddr(runtime_param, op_desc, offset, 0, var_addr), SUCCESS);
  EXPECT_EQ(reinterpret_cast<uint8_t *>(offset), var_addr);
  VarManager::Instance(runtime_param.session_id)->Destory();
}

TEST_F(UtestModelUtils, get_var_addr_rdma_hbm_negative_offset) {
  uint8_t test = 2;
  uint8_t *pf = &test;
  RuntimeParam runtime_param;
  runtime_param.session_id = 0;
  runtime_param.logic_var_base = 0;
  runtime_param.var_base = reinterpret_cast<uintptr_t>(pf);

  int64_t offset = -1;
  EXPECT_EQ(VarManager::Instance(runtime_param.session_id)->Init(0, 0, 0, 0), SUCCESS);
  EXPECT_NE(VarManager::Instance(runtime_param.session_id)->var_resource_, nullptr);
  VarManager::Instance(runtime_param.session_id)->var_resource_->var_offset_map_[offset] = RT_MEMORY_RDMA_HBM;
  std::shared_ptr<OpDesc> op_desc = std::make_shared<OpDesc>("test", "test");
  void *var_addr = nullptr;
  EXPECT_NE(ModelUtils::GetVarAddr(runtime_param, op_desc, offset, 0, var_addr), SUCCESS);
  VarManager::Instance(runtime_param.session_id)->Destory();
}

TEST_F(UtestModelUtils, test_GetInputDataAddrs_input_const) {
  RuntimeParam runtime_param;
  uint8_t weight_base_addr = 0;
  runtime_param.session_id = 0;
  runtime_param.weight_base = reinterpret_cast<uintptr_t>(&weight_base_addr);
  runtime_param.weight_size = 64;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = CreateNode(*graph, "add", ADD, 2, 1);
  auto op_desc = add_node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  vector<bool> is_input_const = {true, true};
  op_desc->SetIsInputConst(is_input_const);
  {
    auto tensor_desc = op_desc->MutableInputDesc(0);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 64);
    tensor_desc->SetShape(GeShape({1, 1}));
    tensor_desc->SetOriginShape(GeShape({1, 1}));
    TensorUtils::SetDataOffset(*tensor_desc, 0);
  }
  {
    auto tensor_desc = op_desc->MutableInputDesc(1);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 32);
    tensor_desc->SetShape(GeShape({1, 0}));
    tensor_desc->SetOriginShape(GeShape({1, 0}));
    TensorUtils::SetDataOffset(*tensor_desc, 64);
  }
  vector<void *> input_data_addr = ModelUtils::GetInputAddrs(runtime_param, op_desc);
  EXPECT_EQ(input_data_addr.size(), 2);
  EXPECT_EQ(input_data_addr.at(0), static_cast<void *>(&weight_base_addr + 0));
  EXPECT_EQ(input_data_addr.at(1), static_cast<void *>(&weight_base_addr + 64));
}

TEST_F(UtestModelUtils, GetInputDataAddrs_tensor_mem_type) {
  RuntimeParam runtime_param;
  runtime_param.session_id = 0;
  runtime_param.mem_size = 0x20002000u;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = CreateNode(*graph, "add", ADD, 3, 1);
  auto op_desc = add_node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  const vector<int64_t> v_memory_type{RT_MEMORY_L1, RT_MEMORY_TS, RT_MEMORY_P2P_DDR};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_INPUT_MEM_TYPE_LIST, v_memory_type);
  op_desc->SetInputOffset({0x10001000, 0x10002000, 0x10004000});

  { // RT_MEMORY_L1
    auto tensor_desc = op_desc->MutableInputDesc(0);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 64);
    TensorUtils::SetDataOffset(*tensor_desc, 0x10001000);
  }

  uint8_t ts_mem_base_addr = 0;
  { // RT_MEMORY_TS
    auto tensor_desc = op_desc->MutableInputDesc(1);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 64);
    TensorUtils::SetDataOffset(*tensor_desc, 0x10002000);

    TsMemMall &ts_mem_mall = *runtime_param.ts_mem_mall;
    ts_mem_mall.mem_store_size_[0x10002000] = &ts_mem_base_addr;   // offset is 0
    ts_mem_mall.mem_store_addr_[&ts_mem_base_addr] = 0x10002000;
  }

  uint8_t p2p_mem_base_addr = 0;
  { // RT_MEMORY_P2P_DDR
    auto tensor_desc = op_desc->MutableInputDesc(2);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 32);
    TensorUtils::SetDataOffset(*tensor_desc, 0x10004000);
    AttrUtils::SetInt(tensor_desc, ATTR_NAME_TENSOR_MEM_TYPE, RT_MEMORY_P2P_DDR);

    MemInfo &mem_info = runtime_param.memory_infos[RT_MEMORY_P2P_DDR];
    mem_info.memory_size = 0x20002000u;
    mem_info.logic_memory_base = 0u;
    mem_info.memory_base = &p2p_mem_base_addr;
    mem_info.memory_type = RT_MEMORY_P2P_DDR;
  }

  const vector<void *> input_data_addr = ModelUtils::GetInputDataAddrs(runtime_param, op_desc);
  EXPECT_EQ(input_data_addr.size(), 3);
  EXPECT_EQ(input_data_addr.at(0), reinterpret_cast<void *>(0x10001000));
  EXPECT_EQ(input_data_addr.at(1), static_cast<void *>(&ts_mem_base_addr));
  EXPECT_EQ(input_data_addr.at(2), static_cast<void *>(&p2p_mem_base_addr + 0x10004000));

  runtime_param.ts_mem_mall->mem_store_size_.clear();
  runtime_param.ts_mem_mall->mem_store_addr_.clear();
}

TEST_F(UtestModelUtils, GetOutputDataAddrs_tensor_mem_type) {
  RuntimeParam runtime_param;
  runtime_param.session_id = 0;
  runtime_param.mem_size = 0x20002000u;

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = CreateNode(*graph, "add", ADD, 1, 4);
  auto op_desc = add_node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  const vector<int64_t> v_memory_type{RT_MEMORY_L1, RT_MEMORY_TS, RT_MEMORY_P2P_DDR, RT_MEMORY_HBM};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, v_memory_type);
  op_desc->SetOutputOffset({0x10001000, 0x10002000, 0x10004000, 0x10008000});

  { // RT_MEMORY_L1
    auto tensor_desc = op_desc->MutableOutputDesc(0);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 64);
    TensorUtils::SetDataOffset(*tensor_desc, 0x10001000);
  }

  uint8_t ts_mem_base_addr = 0;
  { // RT_MEMORY_TS
    auto tensor_desc = op_desc->MutableOutputDesc(1);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 64);
    TensorUtils::SetDataOffset(*tensor_desc, 0x10002000);

    TsMemMall &ts_mem_mall = *runtime_param.ts_mem_mall;
    ts_mem_mall.mem_store_size_[0x10002000] = &ts_mem_base_addr;   // offset is 0
    ts_mem_mall.mem_store_addr_[&ts_mem_base_addr] = 0x10002000;
  }

  uint8_t p2p_mem_base_addr = 0;
  { // RT_MEMORY_P2P_DDR
    auto tensor_desc = op_desc->MutableOutputDesc(2);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 32);
    TensorUtils::SetDataOffset(*tensor_desc, 0x10004000);
    AttrUtils::SetInt(tensor_desc, ATTR_NAME_TENSOR_MEM_TYPE, RT_MEMORY_P2P_DDR);

    MemInfo &mem_info = runtime_param.memory_infos[RT_MEMORY_P2P_DDR];
    mem_info.memory_size = 0x20002000u;
    mem_info.logic_memory_base = 0u;
    mem_info.memory_base = &p2p_mem_base_addr;
    mem_info.memory_type = RT_MEMORY_P2P_DDR;
  }
  uint8_t hbm_mem_base_addr = 0;
  { // RT_MEMORY_HBM_DDR
    auto tensor_desc = op_desc->MutableOutputDesc(3);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 32);
    runtime_param.mem_base = reinterpret_cast<uintptr_t>(&hbm_mem_base_addr);
  }
  const vector<void *> output_data_addr = ModelUtils::GetOutputDataAddrs(runtime_param, op_desc);
  EXPECT_EQ(output_data_addr.size(), 4);
  EXPECT_EQ(output_data_addr.at(0), reinterpret_cast<void *>(0x10001000));
  EXPECT_EQ(output_data_addr.at(1), static_cast<void *>(&ts_mem_base_addr));
  EXPECT_EQ(output_data_addr.at(2), static_cast<void *>(&p2p_mem_base_addr + 0x10004000));
  EXPECT_EQ(output_data_addr.at(3), static_cast<void *>(&hbm_mem_base_addr + 0x10008000));

  runtime_param.ts_mem_mall->mem_store_size_.clear();
  runtime_param.ts_mem_mall->mem_store_addr_.clear();
}

TEST_F(UtestModelUtils, GetWorkspaceDataAddrs_tensor_mem_type) {
  RuntimeParam runtime_param;
  uint8_t mem_base_addr = 0;
  runtime_param.session_id = 0;
  runtime_param.mem_size = 0x20002000u;
  runtime_param.mem_base = reinterpret_cast<uintptr_t>(&mem_base_addr);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = CreateNode(*graph, "add", ADD, 2, 1);
  auto op_desc = add_node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  op_desc->SetWorkspace({0x10001000, 0x10002000, 0, 0x10004000, 0x10008000});
  op_desc->SetWorkspaceBytes({0x1000, 0x2000, 0, 0x1000, 0x3000});

  const vector<int64_t> v_memory_type{RT_MEMORY_L1, RT_MEMORY_P2P_DDR, RT_MEMORY_HBM, RT_MEMORY_HBM};
  AttrUtils::SetListInt(op_desc, TVM_ATTR_NAME_WORKSPACE_TYPE, v_memory_type);
  AttrUtils::SetListInt(op_desc, ATTR_NAME_WORKSPACE_TYPE_LIST, v_memory_type);

  const vector<int32_t> workspace_no_reuse_scope{0, 0, 0, 0};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_WORKSPACE_MEMORY_NO_REUSE_SCOPE, workspace_no_reuse_scope);

  uint8_t p2p_mem_base_addr = 0;
  { // RT_MEMORY_P2P_DDR
    MemInfo &mem_info = runtime_param.memory_infos[RT_MEMORY_P2P_DDR];
    mem_info.memory_size = 0x20002000u;
    mem_info.logic_memory_base = 0u;
    mem_info.memory_base = &p2p_mem_base_addr;
    mem_info.memory_type = RT_MEMORY_P2P_DDR;
  }

  uint8_t session_scope_mem_base_addr = 0;
  { // kSessionScopeMemory
    MemInfo &mem_info = runtime_param.memory_infos[0x100000000u | RT_MEMORY_HBM];
    mem_info.memory_size = 0x20002000u;
    mem_info.logic_memory_base = 0u;
    mem_info.memory_base = &session_scope_mem_base_addr;
  }

  const vector<void *> workspace_addr = ModelUtils::GetWorkspaceDataAddrs(runtime_param, op_desc);
  EXPECT_EQ(workspace_addr.size(), 5);
  EXPECT_EQ(workspace_addr.at(0), reinterpret_cast<void *>(0x10001000));
  EXPECT_EQ(workspace_addr.at(1), static_cast<void *>(&p2p_mem_base_addr + 0x10002000));
  EXPECT_EQ(workspace_addr.at(2), nullptr);
  EXPECT_EQ(workspace_addr.at(3), static_cast<void *>(&session_scope_mem_base_addr + 0x10004000));
  EXPECT_EQ(workspace_addr.at(4), static_cast<void *>(&mem_base_addr + 0x10008000));
}

TEST_F(UtestModelUtils, test_get_weight_size) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = CreateNode(*graph, "add", ADD, 2, 2);
  auto op_desc = add_node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  const auto weight_size = ModelUtils::GetWeightSize(op_desc);
  EXPECT_EQ(weight_size.size(), 0);

  const auto weight_tensor = ModelUtils::GetWeights(op_desc);
  EXPECT_EQ(weight_tensor.size(), 0);

  const auto input_descs = ModelUtils::GetInputDescs(op_desc);
  EXPECT_EQ(input_descs.size(), 2);

  const auto output_descs = ModelUtils::GetOutputDescs(op_desc);
  EXPECT_EQ(input_descs.size(), 2);
}

TEST_F(UtestModelUtils, test_get_string_output_size) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = CreateNode(*graph, "add", ADD, 2, 1);
  auto op_desc = add_node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  auto tensor_desc = op_desc->MutableOutputDesc(0);
  EXPECT_NE(tensor_desc, nullptr);
  tensor_desc->SetDataType(DT_STRING);

  int64_t max_size = 100000;
  AttrUtils::SetInt(op_desc, "_op_max_size", max_size);

  std::vector<int64_t> v_max_size({max_size});
  EXPECT_EQ(ModelUtils::GetOutputSize(op_desc), v_max_size);
}
}  // namespace ge
