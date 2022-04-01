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

#define private public
#define protected public
#include "graph/load/model_manager/task_info/dsa_task_info.h"
#include "graph/load/model_manager/davinci_model.h"
#include "ut/ge/ffts_plus_proto_tools.h"

namespace ge {
class UtestDSATask : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

// test Init_DSATaskInfo
TEST_F(UtestDSATask, init_dsa_task_info) {
  domi::TaskDef task_def;
  DSATaskInfo task_info;
  EXPECT_EQ(task_info.Init(task_def, nullptr), PARAM_INVALID);

  DavinciModel model(0, nullptr);
  task_def.set_stream_id(0);
  EXPECT_EQ(task_info.Init(task_def, &model), FAILED);

  model.stream_list_.push_back((void *)0x12345);
  model.runtime_param_.mem_size = 10240;
  model.runtime_param_.mem_base = reinterpret_cast<uintptr_t>(new uint8_t[model.runtime_param_.mem_size]);
  domi::DSATaskDef *dsa_task = task_def.mutable_dsa_task();
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr add_node = CreateNode(*graph, "add", ADD, 3, 1);
  auto op_desc = add_node->GetOpDesc();
  EXPECT_NE(op_desc, nullptr);

  const vector<int64_t> v_memory_type{RT_MEMORY_L1};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_OUTPUT_MEM_TYPE_LIST, v_memory_type);
  op_desc->SetOutputOffset({2048});
  {  // RT_MEMORY_L1
    auto tensor_desc = op_desc->MutableOutputDesc(0);
    EXPECT_NE(tensor_desc, nullptr);
    TensorUtils::SetSize(*tensor_desc, 64);
    TensorUtils::SetDataOffset(*tensor_desc, 2048);
  }

  const vector<int64_t> v_memory_type1{RT_MEMORY_L1, RT_MEMORY_L1, RT_MEMORY_L1};
  AttrUtils::SetListInt(op_desc, ATTR_NAME_INPUT_MEM_TYPE_LIST, v_memory_type1);
  op_desc->SetInputOffset({2048, 2048, 2048});
  
  {  // RT_MEMORY_L1
    for (int i = 0; i < 3; ++i) {
      auto tensor_desc = op_desc->MutableOutputDesc(0);
      EXPECT_NE(tensor_desc, nullptr);
      TensorUtils::SetSize(*tensor_desc, 64);
      TensorUtils::SetDataOffset(*tensor_desc, 2048);
    }
  }

  const vector<int64_t> v_memory_type2{RT_MEMORY_HBM, RT_MEMORY_HBM};
  op_desc->SetWorkspace({1308, 1458});
  op_desc->SetWorkspaceBytes({150, 150});
  AttrUtils::SetListInt(op_desc, TVM_ATTR_NAME_WORKSPACE_TYPE, v_memory_type2);
  AttrUtils::SetListInt(op_desc, ATTR_NAME_WORKSPACE_TYPE_LIST, v_memory_type2);
  model.op_list_[0] = op_desc;

  dsa_task->set_op_index(0);
  dsa_task->set_start(1);
  dsa_task->set_sqe_type(1);
  dsa_task->set_distribution_type(1);
  dsa_task->set_data_type(1);
  dsa_task->set_alg_type(1);
  dsa_task->set_input_vld(1);
  dsa_task->set_input_value_addr_flag(1);
  dsa_task->set_input1_value_or_ptr(0);
  dsa_task->set_input2_value_or_ptr(0);
  dsa_task->set_seed_value_or_ptr(0);
  dsa_task->set_random_count_value_or_ptr(0);
  domi::DSATaskArgsDef *dsa_task_args = dsa_task->mutable_args();
  dsa_task_args->set_output_addr(0x12);
  dsa_task_args->set_workspace_philox_count_addr(0x24);
  dsa_task_args->set_workspace_input_addr(0x457);
  dsa_task_args->set_seed_value_or_addr("5");
  dsa_task_args->set_input1_value_or_addr("1");
  dsa_task_args->set_input2_value_or_addr("2");

  EXPECT_EQ(task_info.Init(task_def, &model), SUCCESS);

  dsa_task->set_input1_value_or_ptr(1);
  EXPECT_EQ(task_info.Init(task_def, &model), SUCCESS);

  delete [] reinterpret_cast<uint8_t *>(model.runtime_param_.mem_base);
  model.runtime_param_.mem_base = 0U;
  model.stream_list_.clear();
}

TEST_F(UtestDSATask, testDistribute) {
  DSATaskInfo task_info;

  domi::TaskDef task_def;
  task_def.set_stream_id(0);

  DavinciModel model(0, nullptr);
  model.stream_list_.push_back((void *)0x12345);
  task_info.Init(task_def, &model);

  auto ret = task_info.Distribute();
  EXPECT_EQ(ret, SUCCESS);
  model.stream_list_.clear();
}
}  // namespace ge

