/**
 * Copyright 2021-2021 Huawei Technologies Co., Ltd
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#define private public
#define protected public
#include "framework/common/taskdown_common.h"
#include "hybrid/executor/rt_callback_manager.h"
#include "hybrid/executor/subgraph_context.h"
#include "hybrid/node_executor/aicore/aicore_node_executor.h"
#include "init/gelib.h"
#include "graph/bin_cache/node_bin_selector_factory.h"
#include "opskernel_manager/ops_kernel_builder_manager.h"
#include "opskernel_manager/ops_kernel_manager.h"
#undef private
#undef protected
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/graph.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;
namespace {
  // for fuzz compiler to init
class FakeOpsKernelInfoStore : public OpsKernelInfoStore {
 private:
  Status Initialize(const std::map<std::string, std::string> &options) override {
    return SUCCESS;
  };
  Status Finalize() override {
    return SUCCESS;
  };
  bool CheckSupported(const OpDescPtr &op_desc, std::string &reason) const override {
    return false;
  };
  void GetAllOpsKernelInfo(std::map<std::string, ge::OpInfo> &infos) const override{};

  Status FuzzCompileOp(std::vector<NodePtr> &node_vec) override {
    return SUCCESS;
  };
};

void BuildDefaultTaskDef(domi::TaskDef & task_def) {
  task_def.set_type(RT_MODEL_TASK_KERNEL);
  std::vector<uint8_t> args(100, 0);
  task_def.mutable_kernel()->set_args(args.data(), args.size());
  task_def.mutable_kernel()->set_args_size(100);
  task_def.mutable_kernel()->mutable_context()->set_kernel_type(static_cast<uint32_t>(ccKernelType::TE));
  uint16_t args_offset = 20;
  task_def.mutable_kernel()->mutable_context()->set_args_offset(&args_offset, sizeof(args_offset));
}

class FakeOpsKernelBuilder : public OpsKernelBuilder {
 public:
  FakeOpsKernelBuilder() = default;

 private:
  Status Initialize(const map<std::string, std::string> &options) override {
    return SUCCESS;
  };
  Status Finalize() override {
    return SUCCESS;
  };
  Status CalcOpRunningParam(Node &node) override {
    return SUCCESS;
  };
  Status GenerateTask(const Node &node, RunContext &context, std::vector<domi::TaskDef> &tasks) override {
    domi::TaskDef task_def;
    BuildDefaultTaskDef(task_def);
    tasks.push_back(task_def);
    return SUCCESS;
  };
};

void BuildDefaultNodeItem(const NodePtr &node, std::unique_ptr<NodeItem> &node_item) {
  ASSERT_EQ(NodeItem::Create(node, node_item), SUCCESS);
  node_item->input_start = 0;
  node_item->output_start = 0;
  node_item->is_dynamic = true;
  node_item->shape_inference_type = DEPEND_SHAPE_RANGE;
  ASSERT_EQ(node_item->Init(), SUCCESS);
}

class FuzzFailToCompileFusedNodeSelector : public NodeBinSelector {
  public:
  FuzzFailToCompileFusedNodeSelector() {};
  NodeCompileCacheItem *SelectBin(const NodePtr &node, const GEThreadLocalContext *ge_context,
                                  std::vector<domi::TaskDef> &task_defs) override {
    return nullptr;
  };
  Status Initialize() override {
    return SUCCESS;
  };
  private:
  NodeCompileCacheItem cci_;
};
} // namespace

class UtestAiCoreNodeExecutor : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

static ge::OpDescPtr CreateOpDesc(string name = "", string type = "", int in_num = 0, int out_num = 0) {
  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  op_desc->SetStreamId(0);
  static int32_t index = 0;
  op_desc->SetId(index++);

  GeTensorDesc tensor(GeShape(), FORMAT_ND, DT_INT64);
  TensorUtils::SetSize(tensor, 64);
  vector<int64_t> input_offset;
  for (int i = 0; i < in_num; ++i) {
    op_desc->AddInputDesc(tensor);
    input_offset.emplace_back(index * 64 + i * 64);
  }
  op_desc->SetInputOffset(input_offset);

  vector<int64_t> output_offset;
  for (int i = 0; i < out_num; ++i) {
    op_desc->AddOutputDesc(tensor);
    output_offset.emplace_back(index * 64 + in_num * 64 + i * 64);
  }
  op_desc->SetOutputOffset(output_offset);

  op_desc->SetWorkspace({});
  op_desc->SetWorkspaceBytes({});

  ge::AttrUtils::SetStr(op_desc, ge::TVM_ATTR_NAME_MAGIC, "RT_DEV_BINARY_MAGIC_ELF_AIVEC");
  bool support_dynamic = true;
  ge::AttrUtils::GetBool(op_desc, "support_dynamicshape", support_dynamic);
  return op_desc;
}

TEST_F(UtestAiCoreNodeExecutor, callback_success) {
  dlog_setlevel(0, 0, 0);
  std::unique_ptr<AiCoreOpTask> task1(new AiCoreOpTask());
  OpDescPtr op_desc = CreateOpDesc("Add", "Add", 2, 1);
  ge::AttrUtils::SetInt(*op_desc, ge::ATTR_NAME_UNKNOWN_SHAPE_TYPE, DEPEND_SHAPE_RANGE);
  domi::TaskDef task_def;
  BuildDefaultTaskDef(task_def);
  auto graph = make_shared<ComputeGraph>("graph");
  auto node = graph->AddNode(op_desc);
  EXPECT_EQ(task1->Init(node, task_def), ge::SUCCESS);

  std::unique_ptr<NodeItem> new_node;
  BuildDefaultNodeItem(node, new_node);

  GraphItem graph_item;
  graph_item.node_items_.emplace_back(new_node.get());
  graph_item.total_inputs_ = 2;
  graph_item.total_outputs_ = 1;

  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_name");
  HybridModel hybrid_model(ge_root_model);


  GraphExecutionContext graph_context;
  graph_context.model = &hybrid_model;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  graph_context.callback_manager = new (std::nothrow) RtCallbackManager();
  graph_context.own_callback_manager = true;

  auto node_state = subgraph_context.GetNodeState(new_node.get());
  ASSERT_NE(node_state, nullptr);
  auto outputs_shape = reinterpret_cast<uint32_t(*)[9]>(task1->shape_buffer_->GetData());
  outputs_shape[0][0] = 2;
  outputs_shape[0][1] = 1;
  outputs_shape[0][2] = 2;
  std::vector<std::unique_ptr<AiCoreOpTask>> tasks;
  tasks.emplace_back(std::move(task1));
  std::unique_ptr<AiCoreNodeTask> aicore_node_task;
  aicore_node_task.reset(new (std::nothrow) AiCoreNodeTask(std::move(tasks)));
  node_state->GetTaskContext()->execution_context_->dump_properties.is_train_op_debug_ = true;
  ASSERT_EQ(aicore_node_task->ExecuteAsync(*node_state->GetTaskContext(), nullptr), SUCCESS);

  AiCoreNodeExecutor executor;
  std::shared_ptr<NodeTask> cur_task;
  hybrid_model.task_defs_[node] = std::vector<domi::TaskDef>({task_def});
  hybrid_model.node_items_[node] = std::move(new_node);
  hybrid_model.SetNodeBinMode(kOneNodeSingleBinMode);
  ASSERT_EQ(executor.LoadTask(hybrid_model, node, cur_task), SUCCESS);
  executor.PrepareTask(*cur_task, *node_state->GetTaskContext());

  ASSERT_EQ(aicore_node_task->UpdateArgs(*node_state->GetTaskContext()), SUCCESS);
  ASSERT_EQ(aicore_node_task->UpdateTilingData(*node_state->GetTaskContext()), SUCCESS);

  std::unique_ptr<NodeTask> aicpu_task;

  aicore_node_task->aicpu_task_ = std::move(aicpu_task);
  ASSERT_EQ(aicore_node_task->aicpu_task_, nullptr);

  // test select bin
  auto selector = NodeBinSelectorFactory::GetInstance().GetNodeBinSelector(kOneNodeSingleBinMode);
  ASSERT_NE(selector, nullptr);
  auto ret = selector->Initialize();
  ASSERT_EQ(ret, SUCCESS);
  (void)aicore_node_task->SetBinSelector(selector);
  ret = aicore_node_task->SelectBin(*node_state->GetTaskContext(), subgraph_context.GetExecutionContext());
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(aicore_node_task->last_bin_, UINT64_MAX); // single mode not upate op task

  std::pair<rtEvent_t, std::pair<rtCallback_t, void *>> entry;
  auto callback_manager =
      dynamic_cast<RtCallbackManager *>(node_state->GetTaskContext()->execution_context_->callback_manager);
  callback_manager->callback_queue_.Pop(entry);
  auto cb_func = entry.second.first;
  auto cb_args = entry.second.second;
  cb_func(cb_args);
  dlog_setlevel(0, 3, 0);
}

TEST_F(UtestAiCoreNodeExecutor, test_Load_Task_without_task_def) {
  map<std::string, std::string> options;
  options.emplace(BUILD_MODE, BUILD_MODE_TUNING);
  options.emplace(BUILD_STEP, BUILD_STEP_AFTER_BUILDER);
  Status ret = ge::GELib::Initialize(options);
  ASSERT_EQ(ret, SUCCESS);
  OpsKernelInfoStorePtr kernel_ptr = std::make_shared<FakeOpsKernelInfoStore>();
  OpsKernelManager::GetInstance().ops_kernel_store_["AIcoreEngine"] = kernel_ptr;
  OpsKernelBuilderRegistry::GetInstance().kernel_builders_["AIcoreEngine"] = std::make_shared<FakeOpsKernelBuilder>();

  // 1. build hybrid model and node
  OpDescPtr op_desc = CreateOpDesc("conv2d", CONV2D, 1, 1);
  auto graph = make_shared<ComputeGraph>("graph");
  auto conv2d_node = graph->AddNode(op_desc);
  conv2d_node->GetOpDesc()->SetOpKernelLibName("AIcoreEngine");
  // set compile info on node
  ge::AttrUtils::SetStr(conv2d_node->GetOpDesc(), "compile_info_key", "op_compile_info_key");
  ge::AttrUtils::SetStr(conv2d_node->GetOpDesc(), "compile_info_json", "op_compile_info_json");

  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_unknown_rank");
  
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.node_bin_mode_ = kOneNodeMultipleBinsMode;
  hybrid_model.task_defs_[conv2d_node] = {}; //empty task def
  std::unique_ptr<NodeItem> node_item;
  BuildDefaultNodeItem(conv2d_node, node_item);

  // 2. build task_context with node_state
  GraphExecutionContext graph_context;
  graph_context.model = &hybrid_model;
  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item.get());
  graph_item.total_inputs_ = 1;
  graph_item.total_outputs_ = 1;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  auto node_state = subgraph_context.GetNodeState(node_item.get());
  ASSERT_NE(node_state, nullptr);
  // build subgraph_item for fused graph, and set to hybrid model
  hybrid_model.node_items_[conv2d_node] = std::move(node_item);

  // 3. load empty task when fuzz compile
  std::shared_ptr<NodeTask> node_task_after_load;
  AiCoreNodeExecutor executor;
  ASSERT_EQ(executor.LoadTask(hybrid_model, conv2d_node, node_task_after_load), SUCCESS);

  // 4. test select bin
  ASSERT_EQ(node_task_after_load->SelectBin(*node_state->GetTaskContext(), subgraph_context.GetExecutionContext()), SUCCESS);

  // 5. load empty task when norma case, load failed
  hybrid_model.node_bin_mode_ = kOneNodeSingleBinMode;
  ASSERT_NE(executor.LoadTask(hybrid_model, conv2d_node, node_task_after_load), SUCCESS);
}
// test fused node (conv2d)
// before fused, its origin graph is : (conv2d->sqrt) 
// 1.加载aicore task时，加载fused task
// 2.selectbin时，使能fused task
// 3.execute async时，执行fused task
TEST_F(UtestAiCoreNodeExecutor, test_execute_on_origin_fused_task) {
  // 1. test load task with fused_task
  // (1) build origin fused graph
  DEF_GRAPH(fused_graph) {
      auto data = OP_CFG(DATA).Attr(ATTR_NAME_PARENT_NODE_INDEX, 0);
      CHAIN(NODE("data", data)->NODE("conv2d", EXPANDDIMS)->NODE("sqrt", RESHAPE)->NODE("netoutput", NETOUTPUT));
  };
  auto origin_fused_graph = ToComputeGraph(fused_graph);
  auto net_output = origin_fused_graph->FindNode("netoutput");
  AttrUtils::SetInt(net_output->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_PARENT_NODE_INDEX, 0);

  // (2) build hybrid model and node
  OpDescPtr op_desc = CreateOpDesc("fused_conv2d", CONV2D, 1, 1);
  AttrUtils::SetGraph(op_desc, "_original_fusion_graph", origin_fused_graph);
  auto graph = make_shared<ComputeGraph>("graph");
  auto fused_conv2d_node = graph->AddNode(op_desc);

  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->SetModelName("test_fusion");
  
  HybridModel hybrid_model(ge_root_model);
  hybrid_model.node_bin_mode_ = kOneNodeMultipleBinsMode;
  domi::TaskDef task_def;
  BuildDefaultTaskDef(task_def);
  hybrid_model.task_defs_[fused_conv2d_node] = std::vector<domi::TaskDef>({task_def});
  std::unique_ptr<NodeItem> node_item;
  BuildDefaultNodeItem(fused_conv2d_node, node_item);

  // (3) build task_context with node_state
  GraphExecutionContext graph_context;
  graph_context.model = &hybrid_model;
  GraphItem graph_item;
  graph_item.node_items_.emplace_back(node_item.get());
  graph_item.total_inputs_ = 1;
  graph_item.total_outputs_ = 1;
  SubgraphContext subgraph_context(&graph_item, &graph_context);
  ASSERT_EQ(subgraph_context.Init(), SUCCESS);
  auto node_state = subgraph_context.GetNodeState(node_item.get());
  ASSERT_NE(node_state, nullptr);
  // build subgraph_item for fused graph, and set to hybrid model
  hybrid_model.node_items_[fused_conv2d_node] = std::move(node_item);
  std::unique_ptr<GraphItem> origin_fused_graph_item = std::unique_ptr<GraphItem>(new GraphItem());
  NodeExecutorManager::GetInstance().EnsureInitialized();
  for (const auto &node : origin_fused_graph->GetDirectNode()) {
      if (node->GetType() == DATA) {
        ++origin_fused_graph_item->total_inputs_;
      }
      if (node->GetType() == NETOUTPUT) {
        ++origin_fused_graph_item->total_outputs_;
      }
      std::unique_ptr<NodeItem> node_item_in_sub;
      BuildDefaultNodeItem(node, node_item_in_sub);
      // simulate load task on node in subgraph
      if (node->GetType() != DATA && node->GetType() != NETOUTPUT) {
         node->GetOpDesc()->SetOpKernelLibName("GeLocal");
      }
     
      // NodeExecutorManager::GetInstance().GetExecutor(*node_item_in_sub, node_item_in_sub->node_executor);
      // const auto &node_ptr = node_item_in_sub->node;
      // node_item_in_sub->node_executor->LoadTask(hybrid_model, node_ptr, node_item_in_sub->kernel_task);
      origin_fused_graph_item->node_items_.emplace_back(node_item_in_sub.get());
      hybrid_model.node_items_[node] = std::move(node_item_in_sub);
  }
  hybrid_model.subgraph_items_[origin_fused_graph->GetName()] = std::move(origin_fused_graph_item);

  // (4) load task of fused_node
  std::shared_ptr<NodeTask> node_task_after_load;
  AiCoreNodeExecutor executor;
  ASSERT_EQ(executor.LoadTask(hybrid_model, fused_conv2d_node, node_task_after_load), SUCCESS);
  AiCoreNodeTask *aicore_node_task = dynamic_cast<AiCoreNodeTask *>(node_task_after_load.get());
  ASSERT_NE(aicore_node_task->fused_task_, nullptr); // after load task, fused_task_ has been load

  // 2. test select bin switch to origin fused graph execution
  // (1) set failed_compile_selector to aicore_node_task
  aicore_node_task->SetBinSelector(new FuzzFailToCompileFusedNodeSelector());
  
  // (2) test select bin to switch to orgin_fused_graph_execution
  ASSERT_EQ(aicore_node_task->SelectBin(*node_state->GetTaskContext(), subgraph_context.GetExecutionContext()), SUCCESS);
  ASSERT_EQ(aicore_node_task->origin_fused_graph_exec_, true);
}
}  // namespace ge