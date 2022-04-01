/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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
#include <memory>

#define protected public
#define private public
#include "graph/manager/graph_var_manager.h"
#include "graph/manager/graph_mem_manager.h"
#include "graph/ge_context.h"
#include "graph/utils/tensor_utils.h"
#include "framework/common/types.h"
#include "graph/compute_graph.h"

namespace ge {
class UtestGraphVarManagerTest : public testing::Test {
 protected:
  void SetUp() {
    VarManagerPool::Instance().Destory();
  }
  void TearDown() {
    VarManagerPool::Instance().Destory();
  }
};

TEST_F(UtestGraphVarManagerTest, test_set_memory_malloc_size_no_related_option) {
  const map<string, string> options{};
  EXPECT_EQ(VarManager::Instance(0)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL), SUCCESS);
  EXPECT_EQ(VarManager::Instance(0)->graph_mem_max_size_, floor(1024UL * 1024UL * 1024UL * (26.0f / 32.0f)));
  EXPECT_EQ(VarManager::Instance(0)->var_mem_max_size_, floor(1024UL * 1024UL * 1024UL * (5.0f / 32.0f)));
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 0, 0), SUCCESS);
}

TEST_F(UtestGraphVarManagerTest, test_set_memory_malloc_size_with_user_specify_graph_mem_max_size) {
  const map<string, string> options{{"ge.graphMemoryMaxSize", "536870912"}};
  Status ret = VarManager::Instance(0)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);
  EXPECT_EQ(VarManager::Instance(0)->graph_mem_max_size_, floor(1024UL * 1024UL * 1024UL / 2));
  EXPECT_EQ(VarManager::Instance(0)->var_mem_max_size_, floor(1024UL * 1024UL * 1024UL * (5.0f / 32.0f)));
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphVarManagerTest, test_set_memory_malloc_size_with_user_specify_var_mem_max_size) {
  const map<string, string> options{{"ge.variableMemoryMaxSize", "536870912"}};
  Status ret = VarManager::Instance(0)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);
  EXPECT_EQ(VarManager::Instance(0)->graph_mem_max_size_, floor(1024UL * 1024UL * 1024UL * (26.0f / 32.0f)));
  EXPECT_EQ(VarManager::Instance(0)->var_mem_max_size_, floor(1024UL * 1024UL * 1024UL / 2));
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(UtestGraphVarManagerTest, test_mem_manager_not_set) {
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 0, 0), SUCCESS);

  EXPECT_EQ(VarManager::Instance(0)->MallocVarMemory(1024), FAILED);
  EXPECT_EQ(VarManager::Instance(0)->GetVarMemoryBase(RT_MEMORY_RDMA_HBM), nullptr);
  EXPECT_EQ(VarManager::Instance(0)->GetVarMemoryAddr(nullptr, RT_MEMORY_RDMA_HBM), nullptr);

  GeTensorDesc tensor_desc;
  EXPECT_EQ(VarManager::Instance(0)->AssignVarMem("global_step", tensor_desc, RT_MEMORY_RDMA_HBM), INTERNAL_ERROR);
}

TEST_F(UtestGraphVarManagerTest, test_with_mem_manager) {
  const std::vector<rtMemType_t> memory_types({RT_MEMORY_HBM, RT_MEMORY_P2P_DDR});
  EXPECT_EQ(MemManager::Instance().Initialize(memory_types), SUCCESS);
  VarManager::Instance(0)->SetMemManager(&MemManager::Instance());
  EXPECT_EQ(VarManager::Instance(0)->Init(0, 0, 0, 0), SUCCESS);

  EXPECT_EQ(VarManager::Instance(0)->MallocVarMemory(1024), SUCCESS);
  EXPECT_EQ(VarManager::Instance(0)->GetVarMemoryBase(RT_MEMORY_RDMA_HBM), nullptr);

  uint8_t logic_addr = 0;
  EXPECT_EQ(VarManager::Instance(0)->GetVarMemoryAddr(&logic_addr, RT_MEMORY_RDMA_HBM), &logic_addr);
  EXPECT_NE(VarManager::Instance(0)->GetVarMemoryAddr(&logic_addr, RT_MEMORY_HBM), nullptr);

  // RdmaPoolAllocator block_bin_ not found.
  GeTensorDesc tensor_desc;
  EXPECT_EQ(VarManager::Instance(0)->AssignVarMem("global_step", tensor_desc, RT_MEMORY_RDMA_HBM), INTERNAL_ERROR);
}

TEST_F(UtestGraphVarManagerTest, test_var_manager_serial_deserial) {
  const map<string, string> options{{"ge.graphMemoryMaxSize", "536870912"}};
  Status ret = VarManager::Instance(1)->Init(0x5a, 1, 0, 0x5a5a);
  EXPECT_EQ(ret, SUCCESS);
  ret = VarManager::Instance(1)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);
  EXPECT_EQ(ret, SUCCESS);
  size_t graph_mem_max_size = VarManager::Instance(1)->graph_mem_max_size_;
  size_t var_mem_max_size = VarManager::Instance(1)->var_mem_max_size_;
  size_t var_mem_logic_base = VarManager::Instance(1)->var_mem_logic_base_;
  size_t use_max_mem_size = VarManager::Instance(1)->use_max_mem_size_;
  std::vector<int64_t> s = {1,2,3,4};
  GeShape shape(s);
  GeTensorDesc tensor_desc(shape);
  TensorUtils::SetSize(tensor_desc, shape.GetShapeSize());
  std::string str = "global_step";
  ret = VarManager::Instance(1)->AssignVarMem(str, tensor_desc, RT_MEMORY_HBM);
  EXPECT_EQ(ret, SUCCESS);
  TransNodeInfo trans_node_info;
  VarTransRoad fusion_road;
  fusion_road.emplace_back(trans_node_info);
  VarManager::Instance(1)->SetTransRoad(str, fusion_road);

  VarBroadCastInfo broadcast_info;
  broadcast_info.var_name = "test";
  VarManager::Instance(1)->SaveBroadCastInfo(0, broadcast_info);

  deployer::VarManagerInfo info;
  ret = VarManager::Instance(1)->VarManagerToSerial(1, info);
  EXPECT_EQ(ret, SUCCESS);
  auto session_id = info.session_id();
  EXPECT_EQ(session_id, 1);
  info.set_session_id(2);
  ret = VarManager::Instance(2)->VarManagerToDeserial(2, info);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(VarManager::Instance(2)->graph_mem_max_size_, floor(1024UL * 1024UL * 1024UL / 2));
  EXPECT_EQ(VarManager::Instance(2)->var_mem_max_size_, floor(1024UL * 1024UL * 1024UL * (5.0f / 32.0f)));
  EXPECT_EQ(VarManager::Instance(2)->version_, 0x5a);
  EXPECT_EQ(VarManager::Instance(2)->job_id_, 0x5a5a);
  EXPECT_EQ(VarManager::Instance(2)->graph_mem_max_size_, graph_mem_max_size);
  EXPECT_EQ(VarManager::Instance(2)->var_mem_max_size_, var_mem_max_size);
  EXPECT_EQ(VarManager::Instance(2)->var_mem_logic_base_, var_mem_logic_base);
  EXPECT_EQ(VarManager::Instance(2)->use_max_mem_size_, use_max_mem_size);
  EXPECT_EQ(VarManager::Instance(2)->var_resource_->session_id_, 2);

  EXPECT_EQ(VarManager::Instance(2)->var_resource_->var_offset_map_.size(), 1);
  EXPECT_EQ(VarManager::Instance(2)->var_resource_->var_addr_mgr_map_.size(), 1);
  EXPECT_EQ(VarManager::Instance(2)->var_resource_->cur_var_tensor_desc_map_.size(), 1);

  EXPECT_EQ(VarManager::Instance(2)->var_resource_->IsVarExist(str, tensor_desc), true);
  uintptr_t ptr = reinterpret_cast<uintptr_t>(VarManager::Instance(2)->GetVarMemoryBase(RT_MEMORY_HBM, 0));
  EXPECT_EQ(ptr, 0);
  EXPECT_EQ(VarManager::Instance(2)->mem_resource_map_.size(), 1);
  auto resource_src = VarManager::Instance(1)->mem_resource_map_[RT_MEMORY_HBM];
  auto resource = VarManager::Instance(2)->mem_resource_map_[RT_MEMORY_HBM];
  EXPECT_EQ(resource->var_mem_size_, 1536);
  EXPECT_EQ(resource->var_mem_size_, resource_src->var_mem_size_);

  ret = VarManager::Instance(2)->AssignVarMem("Hello_variable", tensor_desc, RT_MEMORY_HBM);
  EXPECT_EQ(ret, SUCCESS);

  EXPECT_EQ(resource->total_size_, resource_src->total_size_);
  EXPECT_EQ(resource->total_size_, var_mem_max_size);
}

TEST_F(UtestGraphVarManagerTest, var_address_op_fail) {
  uint64_t session_id = 1;
  rtMemType_t memory_type = RT_MEMORY_HBM;
  Status retStatus;
  int64_t retInt;
  std::vector<int64_t> s = {1,2,3,4};
  GeShape shape(s);
  GeTensorDesc tensor_desc(shape);
  TensorUtils::SetSize(tensor_desc, shape.GetShapeSize());

  uint8_t *dev_ptr = nullptr;
  VarResource tmpVarRes(22);
  retStatus = tmpVarRes.GetVarAddr("", tensor_desc, &dev_ptr, memory_type);
  ASSERT_EQ(retStatus, FAILED);
  retStatus = tmpVarRes.GetVarAddr("", tensor_desc, nullptr, memory_type);
  ASSERT_EQ(retStatus, FAILED);

  VarManager tmpVarMng(22);
  retStatus = tmpVarMng.SetVarAddr("", tensor_desc, dev_ptr, memory_type);
  EXPECT_EQ(retStatus, ge::INTERNAL_ERROR);
  retStatus = tmpVarMng.GetVarAddr("", tensor_desc, dev_ptr, memory_type);
  EXPECT_EQ(retStatus, ge::INTERNAL_ERROR);
#if 0
  std::shared_ptr<MemResource> memResPtr = std::make_shared<MemResource>();
  memResPtr.reset();
  tmpVarMng.mem_resource_map_[memory_type] = memResPtr;
  retInt = tmpVarMng.GetVarMemSize(memory_type);
  EXPECT_EQ(retInt, 0);
#endif
  const map<string, string> options{{"ge.graphMemoryMaxSize", "536870912"}};
  Status ret = VarManager::Instance(session_id)->Init(0x5a, session_id, 0, 0x5a5a);
  EXPECT_EQ(ret, SUCCESS);
  ret = VarManager::Instance(session_id)->SetMemoryMallocSize(options, 1024UL * 1024UL * 1024UL);
  EXPECT_EQ(ret, SUCCESS);

  std::string var_name = "global_step";
  ret = VarManager::Instance(session_id)->AssignVarMem(var_name, tensor_desc, memory_type);
  EXPECT_EQ(ret, SUCCESS);

  retStatus = VarManager::Instance(session_id)->var_resource_->SaveVarAddr(var_name,
                                                                           tensor_desc,
                                                                           dev_ptr,
                                                                           memory_type);
  EXPECT_EQ(retStatus, FAILED);

}

TEST_F(UtestGraphVarManagerTest, VarManager_WithNo_varResource) {
  Status retStatus;
  bool retBool;
  rtMemType_t retMemType; 
  VarTransRoad *retVarTransRoad;
  VarManager::Instance(1)->var_resource_.reset();

  ge::GeTensorDesc tensor_desc;
  VarManager::Instance(1)->SetVarIsReady(std::string("a"), tensor_desc);
  retBool = VarManager::Instance(1)->IsVarReady(std::string("a"), tensor_desc);
  EXPECT_EQ(retBool, false);

  retBool = VarManager::Instance(1)->IsVarExist(std::string("a"), tensor_desc);
  EXPECT_EQ(retBool, false);

  deployer::VarManagerInfo info;
  retStatus = VarManager::Instance(1)->VarManagerToSerial(1, info);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);

  VarBroadCastInfo broad_cast_info;
  retStatus = VarManager::Instance(1)->SaveBroadCastInfo(0, broad_cast_info);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);

  ge::OpDescPtr op_desc = std::make_shared<OpDesc>("Add", ADD);
  retStatus = VarManager::Instance(1)->RenewCurVarDesc(std::string("a"), op_desc);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);

  retMemType = VarManager::Instance(1)->GetVarMemType(0);
  EXPECT_EQ(retMemType, RT_MEMORY_RESERVED);

  VarTransRoad trans_road;
  retStatus = VarManager::Instance(1)->SetTransRoad(std::string("a"), trans_road);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);

  retVarTransRoad = VarManager::Instance(1)->GetTransRoad(std::string("a"));
  EXPECT_EQ(retVarTransRoad, nullptr);

  uint32_t graph_id = 0;
  retStatus = VarManager::Instance(1)->GetChangedGraphId(std::string("a"), graph_id);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);
  VarManager::Instance(1)->RemoveChangedGraphId(std::string("a"));

  retStatus = VarManager::Instance(1)->SetAllocatedGraphId(std::string("a"), 0);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);

  retStatus = VarManager::Instance(1)->GetAllocatedGraphId(std::string("a"), graph_id);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);

  std::map<std::string, GeTensorDesc> all_variables;
  retStatus = VarManager::Instance(1)->GetAllVariables(all_variables);
  EXPECT_EQ(retStatus, INTERNAL_ERROR);

  retBool = VarManager::Instance(1)->HasSharedVarMemBetweenBatch();
  EXPECT_EQ(retBool, false);
}

} // namespace ge
