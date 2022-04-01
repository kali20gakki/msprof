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
#include <gmock/gmock.h>
#include <vector>

#define private public
#define protected public
#include "hybrid/node_executor/aicore/aicore_task_builder.h"
#include "hybrid/node_executor/node_executor.h"
#include "aicpu/common/aicpu_task_struct.h"
#include "hybrid/node_executor/aicore/aicore_task_builder.h"
#include "hybrid/node_executor/aicore/aicore_op_task.h"
#include "hybrid/node_executor/aicore/aicore_node_executor.h"
#include "framework/common/types.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;

namespace {
struct AicpuTaskStruct {
  aicpu::AicpuParamHead head;
  uint64_t io_addrp[2];
}__attribute__((packed));
}  // namespace

class UtestAiCoreTaskBuilder : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestAiCoreTaskBuilder, test_load_aicpu_task) {
  auto op_desc = std::make_shared<ge::OpDesc>("topk", "TopK");
  AttrUtils::SetBool(op_desc, "partially_supported", true);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  HybridModel hybrid_model(ge_root_model);

  shared_ptr<domi::ModelTaskDef> model_task_def = make_shared<domi::ModelTaskDef>();
  std::vector<domi::TaskDef> task_defs;

  AicpuTaskStruct args;
  args.head.length = sizeof(args);
  args.head.ioAddrNum = 2;

  domi::TaskDef *task_def1 = model_task_def->add_task();
  task_def1->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def1 = task_def1->mutable_kernel();
  domi::KernelContext *context1 = kernel_def1->mutable_context();
  context1->set_kernel_type(6);    // ccKernelType::AI_CPU
  kernel_def1->set_args(reinterpret_cast<const char *>(&args), args.head.length);
  kernel_def1->set_args_size(args.head.length);
  task_defs.emplace_back(*task_def1);
  hybrid_model.task_defs_[node] = {*task_def1};
  std::unique_ptr<NodeItem> node_item;
  ASSERT_EQ(NodeItem::Create(node, node_item), SUCCESS);
  hybrid_model.node_items_[node] = std::move(node_item);
  hybrid_model.node_items_[node]->num_inputs = 1;
  hybrid_model.node_items_[node]->num_outputs = 1;

  domi::TaskDef *task_def2 = model_task_def->add_task();
  task_def2->set_type(RT_MODEL_TASK_ALL_KERNEL);
  auto kernel_def2 = task_def2->mutable_kernel_with_handle();
  auto context2 = kernel_def2->mutable_context();
  context2->set_kernel_type(2);    // ccKernelType::TE
  task_defs.emplace_back(*task_def2);

  domi::TaskDef *task_def3 = model_task_def->add_task();
  task_def3->set_type(RT_MODEL_TASK_KERNEL);
  domi::KernelDef *kernel_def3 = task_def3->mutable_kernel();
  domi::KernelContext *context3 = kernel_def3->mutable_context();
  context3->set_kernel_type(2);    // ccKernelType::TE
  task_defs.emplace_back(*task_def3);
  std::unique_ptr<AiCoreNodeTask> aicore_task = MakeUnique<AiCoreNodeTask>();
  AiCoreTaskBuilder builder(node, task_defs, hybrid_model, *aicore_task);
  EXPECT_EQ(builder.InitTaskDef(), SUCCESS);
  EXPECT_EQ(builder.LoadAicpuTask(), SUCCESS);

  builder.aicore_task_defs_.clear();
  std::map<std::string, std::map<int64_t, int64_t>> workspace_info;
  workspace_info["test1"] = {{2, 3}};
  workspace_info["test2"] = {{3, 4}};
  builder.op_desc_->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspace_info);
  EXPECT_NE(builder.BuildTask(), SUCCESS);
}

TEST_F(UtestAiCoreTaskBuilder, load_atomic_workspace) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("workspace", DATA);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);
  
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  HybridModel hybrid_model(ge_root_model);
  std::vector<domi::TaskDef> task_defs;
  std::unique_ptr<AiCoreNodeTask> aicore_task;
  AiCoreTaskBuilder builder(node, task_defs, hybrid_model, *aicore_task);

  GeAttrValue::NAMED_ATTRS workspaces;

  GeAttrValue::NamedAttrs workspaces_attrs;
  vector<int> dimTypeList;
  dimTypeList.push_back(1);
  dimTypeList.push_back(2);
  dimTypeList.push_back(3);
  AttrUtils::SetListInt(workspaces_attrs, op_desc->GetName(), dimTypeList);
  AttrUtils::SetNamedAttrs(op_desc, EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspaces_attrs);

  map<string, map<int64_t, int64_t>> workspace_info;
  op_desc->SetExtAttr(EXT_ATTR_ATOMIC_WORKSPACE_INFO, workspace_info);
  EXPECT_EQ(builder.LoadAtomicWorkspace(), ge::SUCCESS);
}
} // namespace ge
