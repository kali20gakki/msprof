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
#include "graph_optimizer/op_compiler/op_compiler.h"
#include "graph_optimizer/op_compiler/op_compiler_normal.h"
#include "graph_optimizer/op_compiler/tbe_json_parse.h"
#include "graph/utils/graph_utils.h"
#include "ops_kernel_store/sub_ops_store.h"
#include "fusion_manager/fusion_manager.h"
#include "adapter/common/op_store_adapter_manager.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "common/configuration.h"
#include "common/sgt_slice_type.h"
#include "common/graph_comm.h"
#include "common/scope_allocator.h"
#include "ops_store/sub_op_info_store.h"
#include "ops_store/ops_kernel_manager.h"
#include "common/fe_type_utils.h"
#include "graph/tuning_utils.h"
#include "graph/ge_local_context.h"
#undef private
#undef protected

using namespace testing;
using namespace fe;
using namespace ge;

using TbeOpStoreAdapterPtr = std::shared_ptr<TbeOpStoreAdapter>;

tune::Status L2FusionOptimizer1(ge::ComputeGraph &, GraphCommPtr, ScopeAllocatorPtr, const string &, AOEOption) {
  return tune::NO_FUSION_STRATEGY;
}
tune::Status FFTSOptimizer1(ge::ComputeGraph &, bool) {
  return tune::NO_FUSION_STRATEGY;
}
tune::Status FFTSOptimizer2(ge::ComputeGraph &, bool) {
  return tune::SUCCESS;
}
tune::Status L2FusionOptimizer2(ge::ComputeGraph &, GraphCommPtr, ScopeAllocatorPtr, const string &, AOEOption) {
  return tune::SUCCESS;
}

tune::Status L1FusionOptimizer1(ge::ComputeGraph &, GraphCommPtr, ScopeAllocatorPtr, const string &, AOEOption) {
  return tune::NO_FUSION_STRATEGY;
}

tune::Status L1FusionOptimizer2(ge::ComputeGraph &, GraphCommPtr, ScopeAllocatorPtr, const string &, AOEOption) {
  return tune::SUCCESS;
}

namespace {
std::map<uint64_t, te::TbeOpInfo> te_task_map;
bool SelectTbeOpFormatStub(const te::TbeOpInfo &tbe_op_info, std::string &format_str) {
  return true;
}
bool CheckTbeSupportedStub(te::TbeOpInfo& opinfo, te::CheckSupportedResult &isSupport, std::string &reason) {
  return true;
}
bool PreBuildTbeOpStub(te::TbeOpInfo& opinfo, uint64_t taskId, uint64_t graphId) {
  std::string op_type;
  opinfo.GetOpType(op_type);
  string op_pattern = "ElemWise";
  if (op_type == "Conv2D") {
    op_pattern = "Convolution";
  }
  opinfo.SetPattern(op_pattern);
  te_task_map.emplace(taskId, opinfo);
  return true;
}
te::LX_QUERY_STATUS GetOpInfoStub(const te::TbeOpInfo &tbeOpInfo, std::string &result) {
  result = "qwer";
  return te::LX_QUERY_SUCC;
}
bool WaitAllFinishedStub(uint64_t graphId, vector<te::FinComTask> &tasks) {
  std::string json_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/te_conv2d_compress.json";
  for (auto &item : te_task_map) {
    te::FinComTask task;
    task.taskId = item.first;
    std::string name;
    std::string op_type;
    item.second.GetName(name);
    item.second.GetOpType(op_type);
    task.teNodeOpDesc = std::make_shared<ge::OpDesc>(name, op_type);
    ge::AttrUtils::SetStr(task.teNodeOpDesc, "json_file_path", json_path);
    ge::AttrUtils::SetStr(task.teNodeOpDesc, COMPILE_INFO_JSON, "compile_info_json");
    ge::AttrUtils::SetStr(task.teNodeOpDesc, COMPILE_INFO_KEY, "compile_info_key");
    tasks.push_back(task);
  }
  return true;
}
}

class STEST_fusion_engine_op_compiler : public testing::Test
{
protected:
  void SetUp()
  {
    op_store_adapter_manager_ptr_ = std::make_shared<OpStoreAdapterManager>();
    tbe_adapter_ptr_ = std::make_shared<TbeOpStoreAdapter>();
    tbe_adapter_ptr_->SelectTbeOpFormat = SelectTbeOpFormatStub;
    tbe_adapter_ptr_->CheckTbeSupported = CheckTbeSupportedStub;
    tbe_adapter_ptr_->PreBuildTbeOp = PreBuildTbeOpStub;
    tbe_adapter_ptr_->GetOpInfo = GetOpInfoStub;
    tbe_adapter_ptr_->WaitAllFinished = WaitAllFinishedStub;
    tbe_adapter_ptr_->InitializeInnerHelp();
    op_store_adapter_manager_ptr_->map_all_op_store_adapter_.emplace(std::make_pair("tbe_op_adapter", tbe_adapter_ptr_));
    ops_kernel_info_store_ptr_ = std::make_shared<FEOpsKernelInfoStore>(op_store_adapter_manager_ptr_);

    FEOpsStoreInfo tbe_custom {
            2,
            "tbe-custom",
            EN_IMPL_CUSTOM_TBE,
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/tbe_custom_opinfo",
            true,
            true};

    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(tbe_custom);
    Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();
    OpsKernelManager::Instance(AI_CORE_NAME).Initialize();

    graph_ = CreateTestGraph();
    graph_cce_ = CreateCceGraph();
    graph_mix_ = CreateMixGraph();
  }

  void TearDown()
  {
    te_task_map.clear();
  }

  static NodePtr CreateCceNode(string name, GeTensorDescPtr tensor_desc_ptr, ComputeGraphPtr graph)
  {
    OpDescPtr other_desc_ptr = std::make_shared<OpDesc>(name, "otherNode");
    //set OpDesc
    auto local_tensor_desc = tensor_desc_ptr->Clone();
    // add two input desc
    for (int i = 0; i < 2; ++i) {
      AttrUtils::SetStr(local_tensor_desc, "name", name + "In" + std::to_string(i));
      other_desc_ptr->AddInputDesc(local_tensor_desc);
    }
    // add two output desc
    for (int i = 0; i < 2; ++i) {
      AttrUtils::SetStr(local_tensor_desc, "name", name + "Out" + std::to_string(i));
      other_desc_ptr->AddOutputDesc(local_tensor_desc);
    }
    // add node from other_desc_ptr to graph
    // set attr
    AttrUtils::SetInt(other_desc_ptr, "T", DT_FLOAT);
    AttrUtils::SetInt(other_desc_ptr, "_fe_imply_type", EN_IMPL_HW_GENERAL_CCE);

    NodePtr node_other = graph->AddNode(other_desc_ptr);

    return node_other;
  }

  static NodePtr CreateOtherNode(string name, GeTensorDescPtr tensor_desc_ptr, ComputeGraphPtr graph)
  {
    OpDescPtr other_desc_ptr = std::make_shared<OpDesc>(name, "otherNode");
    //set OpDesc
    auto local_tensor_desc = tensor_desc_ptr->Clone();
    // add two input desc
    for (int i = 0; i < 2; ++i) {
      AttrUtils::SetStr(local_tensor_desc, "name", name + "In" + std::to_string(i));
      other_desc_ptr->AddInputDesc(local_tensor_desc);
    }
    // add two output desc
    for (int i = 0; i < 2; ++i) {
      AttrUtils::SetStr(local_tensor_desc, "name", name + "Out" + std::to_string(i));
      other_desc_ptr->AddOutputDesc(local_tensor_desc);
    }
    // add node from other_desc_ptr to graph
    // set attr
    AttrUtils::SetInt(other_desc_ptr, "T", DT_FLOAT);
    AttrUtils::SetInt(other_desc_ptr, "_fe_imply_type", EN_IMPL_CUSTOM_TBE);

    NodePtr node_other = graph->AddNode(other_desc_ptr);

    return node_other;
  }

  static ComputeGraphPtr CreateCceGraph()
  {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    // new a output GeTensorDesc
    GeTensorDescPtr general_ge_tensor_desc = std::make_shared<GeTensorDesc>();
    general_ge_tensor_desc->SetFormat(FORMAT_NCHW);
    general_ge_tensor_desc->SetDataType(DT_FLOAT);

    int total_node_num = 4;
    vector<NodePtr> nodes;
    for (int i = 0; i < total_node_num; ++i) {
      nodes.push_back(CreateCceNode("test/other" + std::to_string(i), general_ge_tensor_desc, graph));
    }
    /* add link of anchors */
    std::vector<OutDataAnchorPtr> srcs;
    std::vector<InDataAnchorPtr> dsts;
    for (int i = 0; i < total_node_num - 1; ++i) {
      srcs.push_back(nodes[i]->GetOutDataAnchor(0));
      dsts.push_back(nodes[i + 1]->GetInDataAnchor(0));
      srcs.push_back(nodes[i]->GetOutDataAnchor(1));
      dsts.push_back(nodes[i + 1]->GetInDataAnchor(1));
    }

    // add edges
    for (int i = 0; i < srcs.size(); ++i)
    {
      GraphUtils::AddEdge(srcs[i], dsts[i]);
    }

    return graph;
  }

  static ComputeGraphPtr CreateMixGraph()
  {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    // new a output GeTensorDesc
    GeTensorDescPtr general_ge_tensor_desc = std::make_shared<GeTensorDesc>();
    general_ge_tensor_desc->SetFormat(FORMAT_NCHW);
    general_ge_tensor_desc->SetDataType(DT_FLOAT);

    int total_node_num = 4;
    vector<NodePtr> nodes;
    for (int i = 0; i < 2; ++i) {
      nodes.push_back(CreateOtherNode("test/other" + std::to_string(i), general_ge_tensor_desc, graph));
    }
    for (int i = 2; i < total_node_num; ++i) {
      nodes.push_back(CreateCceNode("test/other" + std::to_string(i), general_ge_tensor_desc, graph));
    }
    /* add link of anchors */
    std::vector<OutDataAnchorPtr> srcs;
    std::vector<InDataAnchorPtr> dsts;
    for (int i = 0; i < total_node_num - 1; ++i) {
      srcs.push_back(nodes[i]->GetOutDataAnchor(0));
      dsts.push_back(nodes[i + 1]->GetInDataAnchor(0));
      srcs.push_back(nodes[i]->GetOutDataAnchor(1));
      dsts.push_back(nodes[i + 1]->GetInDataAnchor(1));
    }

    // add edges
    for (int i = 0; i < srcs.size(); ++i)
    {
      GraphUtils::AddEdge(srcs[i], dsts[i]);
    }

    return graph;
  }

  static ComputeGraphPtr CreateTestGraph()
  {
    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    // new a output GeTensorDesc
    GeTensorDescPtr general_ge_tensor_desc = std::make_shared<GeTensorDesc>();
    general_ge_tensor_desc->SetFormat(FORMAT_NCHW);
    general_ge_tensor_desc->SetDataType(DT_FLOAT);

    int total_node_num = 4;
    vector<NodePtr> nodes;
    for (int i = 0; i < total_node_num; ++i) {
      nodes.push_back(CreateOtherNode("test/other" + std::to_string(i), general_ge_tensor_desc, graph));
    }
    /* add link of anchors */
    std::vector<OutDataAnchorPtr> srcs;
    std::vector<InDataAnchorPtr> dsts;
    for (int i = 0; i < total_node_num - 1; ++i) {
      srcs.push_back(nodes[i]->GetOutDataAnchor(0));
      dsts.push_back(nodes[i + 1]->GetInDataAnchor(0));
      srcs.push_back(nodes[i]->GetOutDataAnchor(1));
      dsts.push_back(nodes[i + 1]->GetInDataAnchor(1));
    }

    // add edges
    for (int i = 0; i < srcs.size(); ++i)
    {
      GraphUtils::AddEdge(srcs[i], dsts[i]);
    }

    return graph;
  }

  static ComputeGraphPtr BuildTestGraph(const int32_t &strategy) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Conv2D");
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", "Conv2D");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "RelU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "RelU");

    int64_t scope_id_1 = ScopeAllocator::Instance().AllocateScopeId();
    int64_t scope_id_2 = ScopeAllocator::Instance().AllocateScopeId();
    int64_t scope_id_3 = ScopeAllocator::Instance().AllocateScopeId();
    switch (strategy) {
      case 1:
        ScopeAllocator::SetScopeAttr(conv1, scope_id_1);
        ScopeAllocator::SetScopeAttr(relu1, scope_id_1);
        ScopeAllocator::SetScopeAttr(conv2, scope_id_2);
        ScopeAllocator::SetScopeAttr(relu2, scope_id_2);

        ScopeAllocator::SetL1ScopeAttr(conv1, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(relu1, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(conv2, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(relu2, scope_id_3);
        break;
      case 2:
        ScopeAllocator::SetScopeAttr(conv1, scope_id_1);
        ScopeAllocator::SetScopeAttr(relu1, scope_id_1);
        ScopeAllocator::SetScopeAttr(conv2, scope_id_2);
        ScopeAllocator::SetScopeAttr(relu2, scope_id_2);
        break;
      case 3:
        ScopeAllocator::SetScopeAttr(conv1, scope_id_1);
        ScopeAllocator::SetScopeAttr(relu1, scope_id_1);
        ScopeAllocator::SetScopeAttr(conv2, scope_id_2);
        ScopeAllocator::SetScopeAttr(relu2, scope_id_2);
        ScopeAllocator::SetL1ScopeAttr(conv1, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(relu1, scope_id_3);
        break;
      case 4:
        ScopeAllocator::SetScopeAttr(conv1, scope_id_1);
        ScopeAllocator::SetScopeAttr(relu1, scope_id_1);
        ScopeAllocator::SetScopeAttr(conv2, scope_id_2);
        ScopeAllocator::SetScopeAttr(relu2, scope_id_2);

        ScopeAllocator::SetL1ScopeAttr(conv2, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(relu2, scope_id_3);
        break;
      case 5:
        ScopeAllocator::SetScopeAttr(conv1, scope_id_1);
        ScopeAllocator::SetScopeAttr(relu1, scope_id_1);
        ScopeAllocator::SetScopeAttr(conv2, scope_id_2);
        ScopeAllocator::SetScopeAttr(relu2, scope_id_2);

        ScopeAllocator::SetL1ScopeAttr(conv2, scope_id_1);
        ScopeAllocator::SetL1ScopeAttr(relu2, scope_id_1);
        break;
      default:
        ScopeAllocator::SetScopeAttr(conv1, scope_id_1);
        ScopeAllocator::SetScopeAttr(relu1, scope_id_1);
        ScopeAllocator::SetScopeAttr(conv2, scope_id_2);
        ScopeAllocator::SetScopeAttr(relu2, scope_id_2);

        ScopeAllocator::SetL1ScopeAttr(conv1, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(relu1, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(conv2, scope_id_3);
        ScopeAllocator::SetL1ScopeAttr(relu2, scope_id_3);
    }

    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    // add descriptor
    vector<int64_t> dim = {4, 4, 1, 4};
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    conv1->AddInputDesc(tenosr_desc);
    conv1->AddInputDesc(tenosr_desc);
    conv1->AddOutputDesc(tenosr_desc);

    conv2->AddInputDesc(tenosr_desc);
    conv2->AddInputDesc(tenosr_desc);
    conv2->AddOutputDesc(tenosr_desc);

    relu1->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);

    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);

    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), conv2_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));

    return graph;
  }

  static ComputeGraphPtr BuildSomeGraph(const bool& is_dynamic, const int32_t &type) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", "Conv2D");
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", "Conv2D");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1", "Relu");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2", "Relu");

    AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, fe::EN_IMPL_CUSTOM_TBE);
    AttrUtils::SetInt(conv2, FE_IMPLY_TYPE, fe::EN_IMPL_CUSTOM_TBE);
    AttrUtils::SetInt(relu1, FE_IMPLY_TYPE, fe::EN_IMPL_CUSTOM_TBE);
    AttrUtils::SetInt(relu2, FE_IMPLY_TYPE, fe::EN_IMPL_CUSTOM_TBE);

    fe::ToOpStructPtr l1_info_ptr = std::make_shared<fe::ToOpStruct_t>();
    std::vector<int64_t> op_input_l1 = {1,2,3,4};
    vector<uint32_t> dynamic_input_start_idx = {0};
    vector<uint32_t> dynamic_input_end_idx = {1};
    switch (type) {
      case 1:
        l1_info_ptr->op_l1_fusion_type = {1};
        l1_info_ptr->op_l1_space = 2;
        l1_info_ptr->op_l1_workspace_flag = 3;
        l1_info_ptr->op_l1_workspace_size = 4;
        l1_info_ptr->slice_input_shape = {{5}};
        l1_info_ptr->slice_input_offset = {{6}};
        l1_info_ptr->slice_output_shape = {{7}};
        l1_info_ptr->slice_output_offset = {{8}};
        l1_info_ptr->total_shape = {9};
        l1_info_ptr->split_index = 0;
        conv1->SetExtAttr(ge::ATTR_NAME_L1_FUSION_EXTEND_PTR, l1_info_ptr);
        conv2->SetExtAttr(fe::ATTR_NAME_L2_FUSION_EXTEND_PTR, l1_info_ptr);
        break;
      case 2:
        ge::AttrUtils::SetListInt(relu1, ge::ATTR_NAME_OP_INPUT_L1_FLAG, op_input_l1);
        ge::AttrUtils::SetListInt(relu1, ge::ATTR_NAME_OP_INPUT_L1_ADDR, op_input_l1);
        ge::AttrUtils::SetListInt(relu1, ge::ATTR_NAME_OP_INPUT_L1_VALID_SIZE, op_input_l1);
        ge::AttrUtils::SetBool(relu2, NEED_RE_PRECOMPILE, true);
        break;
      case 3:
        ge::AttrUtils::SetStr(relu1, ge::ATTR_NAME_UNREGST_OPPATH, "unregst_oppath");
        ge::AttrUtils::SetStr(relu2, ge::ATTR_NAME_UNREGST_OPPATH, "unregst_oppath");
        break;
      case 4:
        ge::AttrUtils::SetStr(conv1, ge::ATTR_NAME_UNREGST_OPPATH, "unregst_oppath");
        ge::AttrUtils::SetStr(conv2, ge::ATTR_NAME_UNREGST_OPPATH, "unregst_oppath");
        ge::AttrUtils::SetListInt(conv1, ge::ATTR_NAME_DYNAMIC_INPUT_START, dynamic_input_start_idx);
        ge::AttrUtils::SetListInt(conv1, ge::ATTR_NAME_DYNAMIC_INPUT_END, dynamic_input_end_idx);
        break;
    }

    // add descriptor
    vector<int64_t> dim = {4, 4, 1, 4};
    if (is_dynamic) {
      dim[1] = -1;
    }
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    conv1->AddInputDesc(tenosr_desc);
    conv1->AddInputDesc(tenosr_desc);
    conv1->AddOutputDesc(tenosr_desc);

    conv2->AddInputDesc(tenosr_desc);
    conv2->AddInputDesc(tenosr_desc);
    conv2->AddOutputDesc(tenosr_desc);

    relu1->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);

    ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);

    GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), conv2_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));

    return graph;
  }

  FEOpsKernelInfoStorePtr ops_kernel_info_store_ptr_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  TbeOpStoreAdapterPtr tbe_adapter_ptr_;
  ComputeGraphPtr graph_;
  ComputeGraphPtr graph_cce_;
  ComputeGraphPtr graph_mix_;
};

TEST_F(STEST_fusion_engine_op_compiler, save_fusion_node_found)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  auto node = graph_->GetDirectNode().at(0);
  AttrUtils::SetInt(node->GetOpDesc(), "fusion_scope", 1);

  int64_t scope_id = 1;

  ScopeNodeIdMap fusion_node_map;
  std::vector<ge::Node*> fusion_nodes;
  fusion_node_map.emplace(std::make_pair(1, fusion_nodes));

  Status status = op_compiler_ptr->AddNodeToFusionMap(*node, scope_id, fusion_node_map);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_op_compiler, save_fusion_node_not_found)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  auto node = graph_->GetDirectNode().at(0);
  AttrUtils::SetInt(node->GetOpDesc(), "fusion_scope", 1);

  int64_t scope_id = 1;

  ScopeNodeIdMap fusion_node_map;
  std::vector<ge::Node*> fusion_nodes;
  fusion_node_map.emplace(std::make_pair(2, fusion_nodes));

  Status status = op_compiler_ptr->AddNodeToFusionMap(*node, scope_id, fusion_node_map);

  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_op_compiler, case_get_scope_node_map_suc)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  std::vector<ge::NodePtr> scope_nodes;
  for (auto &node : graph_->GetDirectNode()) {
  ge::AttrUtils::SetBool(node->GetOpDesc(), ge::ATTR_NAME_NOTASK, true);
  scope_nodes.push_back(node);
  }
  ScopeNodeIdMap fusion_node_map;
  ge::AttrUtils::SetStr(graph_, ge::ATTR_NAME_SESSION_GRAPH_ID, "session_graph_id_0");
  Status ret = op_compiler_ptr->GetScopeNodeMap(*graph_, scope_nodes, fusion_node_map);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, case_run_compile_process_fail)
{
  auto op_compiler_ptr = std::make_shared<OpCompilerNormal>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
  OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");

  // add descriptor
  vector<int64_t> dims = {288, 32, 16, 16};
  GeShape shape(dims);
  GeTensorDesc in_desc1(shape);
  in_desc1.SetFormat(FORMAT_FRACTAL_Z);
  in_desc1.SetDataType(DT_FLOAT16);
  bn_op->AddInputDesc("x", in_desc1);
  GeTensorDesc out_desc1(shape);
  out_desc1.SetFormat(FORMAT_NHWC);
  out_desc1.SetDataType(DT_FLOAT16);
  bn_op->AddOutputDesc("y", out_desc1);
  GeTensorDesc in_desc2(shape);
  in_desc2.SetFormat(FORMAT_NCHW);
  in_desc2.SetDataType(DT_FLOAT16);
  relu_op->AddInputDesc("x", in_desc2);
  GeTensorDesc out_desc2(shape);
  out_desc2.SetFormat(FORMAT_HWCN);
  out_desc2.SetDataType(DT_FLOAT16);
  relu_op->AddOutputDesc("y", out_desc2);
  ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
  ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
  NodePtr bn_node = graph->AddNode(bn_op);
  NodePtr relu_node = graph->AddNode(relu_op);
  GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::vector<ge::NodePtr> buff_fus_compile_failed_nodes;
  bool need_post_process = false;
  OpImplType op_impl_type = EN_IMPL_CUSTOM_TBE;
  for(FEOpsStoreInfo &fe_op_store_info: Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_) {
    if (op_impl_type == fe_op_store_info.op_impl_type) {
      fe_op_store_info.need_compile = false;
    }
  }

  Status ret = op_compiler_ptr->RunCompileProcess(*graph, graph_comm_ptr,
                                                  buff_fus_compile_failed_nodes, need_post_process);
  for(FEOpsStoreInfo &fe_op_store_info: Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_) {
    if (op_impl_type == fe_op_store_info.op_impl_type) {
      fe_op_store_info.need_compile = true;
    }
  }
  EXPECT_EQ(fe::PARAM_INVALID, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, case_run_compile_process_failed1)
{
  auto op_compiler_ptr = std::make_shared<OpCompilerBaseline>("baseline compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
  OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");

  string path = "./air/test/engines/nneng/config/data/platform_config";
  string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().LoadConfigFile(real_path);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend910A";

  // add descriptor
  vector<int64_t> dims = {288, 32, 16, 16};
  GeShape shape(dims);
  GeTensorDesc in_desc1(shape);
  in_desc1.SetFormat(FORMAT_FRACTAL_Z);
  in_desc1.SetDataType(DT_FLOAT16);
  bn_op->AddInputDesc("x", in_desc1);
  GeTensorDesc out_desc1(shape);
  out_desc1.SetFormat(FORMAT_NHWC);
  out_desc1.SetDataType(DT_FLOAT16);
  bn_op->AddOutputDesc("y", out_desc1);
  GeTensorDesc in_desc2(shape);
  in_desc2.SetFormat(FORMAT_NCHW);
  in_desc2.SetDataType(DT_FLOAT16);
  relu_op->AddInputDesc("x", in_desc2);
  GeTensorDesc out_desc2(shape);
  out_desc2.SetFormat(FORMAT_HWCN);
  out_desc2.SetDataType(DT_FLOAT16);
  relu_op->AddOutputDesc("y", out_desc2);
  ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
  ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
  NodePtr bn_node = graph->AddNode(bn_op);
  NodePtr relu_node = graph->AddNode(relu_op);
  GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::vector<ge::NodePtr> buff_fus_compile_failed_nodes;
  bool need_post_process = false;

  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_);
  BufferFusionFunc func = std::bind(&FEGraphOptimizer::BufferFusionProcess, fe_graph_optimizer_ptr, std::placeholders::_1,
                                    std::placeholders::_2, std::placeholders::_3);

  Status ret = op_compiler_ptr->Initialize(func);
  EXPECT_EQ(fe::SUCCESS, ret);

  // init pass mgr ptr
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  ret = fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->Initialize(AI_CORE_NAME);
  EXPECT_EQ(fe::SUCCESS, ret);

  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L1FusionOptimizer = L1FusionOptimizer1;
  Configuration::Instance(AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  ret = op_compiler_ptr->RunCompileProcess(*graph, graph_comm_ptr,
                                           buff_fus_compile_failed_nodes, need_post_process);
  EXPECT_EQ(fe::FAILED, ret);

  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L1FusionOptimizer = L1FusionOptimizer2;
  Configuration::Instance(AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  ret = op_compiler_ptr->RunCompileProcess(*graph, graph_comm_ptr,
                                           buff_fus_compile_failed_nodes, need_post_process);
  EXPECT_EQ(fe::FAILED, ret);

  ret = op_compiler_ptr->Initialize(func);
  EXPECT_EQ(fe::SUCCESS, ret);
  Configuration::Instance(AI_CORE_NAME).buffer_fusion_mode_ = EN_L2_FUSION;
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L2FusionOptimizer = L2FusionOptimizer1;
  ret = op_compiler_ptr->RunCompileProcess(*graph, graph_comm_ptr,
                                           buff_fus_compile_failed_nodes, need_post_process);
  EXPECT_EQ(fe::FAILED, ret);

  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L2FusionOptimizer = L2FusionOptimizer2;
  ret = op_compiler_ptr->RunCompileProcess(*graph, graph_comm_ptr,
                                           buff_fus_compile_failed_nodes, need_post_process);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, has_compile_strategy_suc)
{
  auto op_compiler_ptr = std::make_shared<OpCompilerNormal>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
  OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");
  ge::AttrUtils::SetStr(bn_op, "_op_compile_strategy", "NO_TUNE");
  ge::AttrUtils::SetStr(relu_op, "_op_compile_strategy", "NO_TUNE");
  NodePtr bn_node = graph->AddNode(bn_op);
  NodePtr relu_node = graph->AddNode(relu_op);
  std::vector<ge::NodePtr> nodes_be_compiled;
  nodes_be_compiled.push_back(bn_node);
  nodes_be_compiled.push_back(relu_node);
  Status ret = op_compiler_ptr->HasCompileStrategy(nodes_be_compiled);
  EXPECT_EQ(true, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, has_compile_strategy_failed)
{
  auto op_compiler_ptr = std::make_shared<OpCompilerNormal>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
  OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");
  NodePtr bn_node = graph->AddNode(bn_op);
  NodePtr relu_node = graph->AddNode(relu_op);
  std::vector<ge::NodePtr> nodes_be_compiled;
  nodes_be_compiled.push_back(bn_node);
  nodes_be_compiled.push_back(relu_node);
  Status ret = op_compiler_ptr->HasCompileStrategy(nodes_be_compiled);
  EXPECT_EQ(false, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, case_run_compile_process_failed2)
{
  auto op_compiler_ptr = std::make_shared<OpCompilerBaseline>("baseline compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");

  OpDescPtr bn_op = std::make_shared<OpDesc>("batchnormal", "BatchNorm");
  OpDescPtr relu_op = std::make_shared<OpDesc>("relu", "Activation");

  // add descriptor
  vector<int64_t> dims = {288, 32, 16, 16};
  GeShape shape(dims);
  GeTensorDesc in_desc1(shape);
  in_desc1.SetFormat(FORMAT_FRACTAL_Z);
  in_desc1.SetDataType(DT_FLOAT16);
  bn_op->AddInputDesc("x", in_desc1);
  GeTensorDesc out_desc1(shape);
  out_desc1.SetFormat(FORMAT_NHWC);
  out_desc1.SetDataType(DT_FLOAT16);
  bn_op->AddOutputDesc("y", out_desc1);
  GeTensorDesc in_desc2(shape);
  in_desc2.SetFormat(FORMAT_NCHW);
  in_desc2.SetDataType(DT_FLOAT16);
  relu_op->AddInputDesc("x", in_desc2);
  GeTensorDesc out_desc2(shape);
  out_desc2.SetFormat(FORMAT_HWCN);
  out_desc2.SetDataType(DT_FLOAT16);
  relu_op->AddOutputDesc("y", out_desc2);
  ge::AttrUtils::SetInt(bn_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
  ge::AttrUtils::SetInt(relu_op, FE_IMPLY_TYPE, static_cast<int>(EN_IMPL_CUSTOM_TBE));
  (void)ge::AttrUtils::SetBool(bn_op, NEED_RE_PRECOMPILE, true);
  (void)ge::AttrUtils::SetBool(relu_op, NEED_RE_PRECOMPILE, true);
  ge::AttrUtils::SetInt(bn_op, L1_SCOPE_ID_ATTR, 123);
  ge::AttrUtils::SetInt(relu_op, L1_SCOPE_ID_ATTR, 123);
  NodePtr bn_node = graph->AddNode(bn_op);
  NodePtr relu_node = graph->AddNode(relu_op);
  GraphUtils::AddEdge(bn_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>("engineName");
  graph_comm_ptr->Initialize();
  std::vector<ge::NodePtr> buff_fus_compile_failed_nodes;
  bool need_post_process = false;

  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_);
  BufferFusionFunc func = std::bind(&FEGraphOptimizer::BufferFusionProcess, fe_graph_optimizer_ptr, std::placeholders::_1,
                                    std::placeholders::_2, std::placeholders::_3);

  Status ret = op_compiler_ptr->Initialize(func);
  EXPECT_EQ(fe::SUCCESS, ret);

  // init pass mgr ptr
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  ret = fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->Initialize(AI_CORE_NAME);
  EXPECT_EQ(fe::SUCCESS, ret);
  CompileInfoParam compile_info(buff_fus_compile_failed_nodes);
  LxFusionOptimizeResult opt_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;
  ret = op_compiler_ptr->ReCompileOpAfterLxFusion(*graph, compile_info, opt_ret);
  EXPECT_EQ(fe::FAILED, ret);
  vector<ge::NodePtr> nodes_be_re_compiled;
  vector<ge::NodePtr> all_nodes;
  ret = op_compiler_ptr->GetFusionScope(*graph, compile_info.fusion_nodes_map, nodes_be_re_compiled, all_nodes);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_thread_op_no_thread_scope_id_suc)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  bool sgt_flag = false;
  Status ret = op_compiler_ptr->PreCompileThreadOp(*graph_, sgt_flag);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_thread_op_has_thread_scope_id_suc)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  for (auto &node : graph_->GetDirectNode()) {
    AttrUtils::SetInt(node->GetOpDesc(), kThreadScopeId, 1);
    ffts::ThreadSliceMap subgraphInfo;
    vector<vector<vector<ffts::DimRange>>> inputTensorSlice;
    vector<vector<vector<int64_t>>> oriInputTensorSlice;
    for (size_t i = 0; i < 2; i++) {
      vector<int64_t> vec1 = {0, 288, 0, 32, 0, 16, 0, 16};
      vector<ffts::DimRange> vdr1;
      for (size_t j = 0; j < vec1.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec1[j];
        dr.higher = vec1[j + 1];
        vdr1.push_back(dr);
        j = j + 2;
      }
      vector<vector<ffts::DimRange>> threadSlice;
      threadSlice.push_back(vdr1);
      threadSlice.push_back(vdr1);

      vector<int64_t> vec2 = {0, 3, 0, 3, 0, 512, 0, 512};
      vector<int64_t> vdr2;
      for (size_t j = 0; j < vec2.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec2[j];
        dr.higher = vec2[j + 1];
        vdr2.push_back(dr.higher - dr.lower);
        j = j + 2;
      }
      vector<vector<int64_t>> oriThreadSlice;
      oriThreadSlice.push_back(vdr2);
      oriThreadSlice.push_back(vdr2);
      inputTensorSlice.push_back(threadSlice);
      oriInputTensorSlice.push_back(oriThreadSlice);
    }

    vector<vector<vector<ffts::DimRange>>> outputTensorSlice;
    vector<vector<vector<int64_t>>> oriOutputTensorSlice;
    for (size_t i = 0; i < 2; i++) {
      vector<int64_t> vec3 = {0, 1, 0, 32, 0, 14, 0, 14, 0, 16};
      vector<ffts::DimRange> vdr3;
      for (size_t j = 0; j < vec3.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec3[j];
        dr.higher = vec3[j + 1];
        vdr3.push_back(dr);
        j = j + 2;
      }
      vector<vector<ffts::DimRange>> threadSlice;
      threadSlice.push_back(vdr3);
      threadSlice.push_back(vdr3);

      vector<int64_t> vec4 = {0, 1, 0, 32, 0, 14, 0, 14, 0, 16};
      vector<int64_t> vdr4;
      for (size_t j = 0; j < vec4.size() - 1;) {
        ffts::DimRange dr;
        dr.lower = vec4[j];
        dr.higher = vec4[j + 1];
        vdr4.push_back(dr.higher - dr.lower);
        j = j + 2;
      }
      vector<vector<int64_t>> oriThreadSlice;
      oriThreadSlice.push_back(vdr4);
      oriThreadSlice.push_back(vdr4);
      outputTensorSlice.push_back(threadSlice);
      oriOutputTensorSlice.push_back(oriThreadSlice);
    }

    subgraphInfo.input_tensor_slice = inputTensorSlice;
    subgraphInfo.ori_input_tensor_shape = oriInputTensorSlice;
    subgraphInfo.output_tensor_slice = outputTensorSlice;
    subgraphInfo.ori_output_tensor_shape = oriOutputTensorSlice;
    ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(subgraphInfo);
    node->GetOpDesc()->SetExtAttr("_sgt_struct_info", tsmp_ptr);
  }
  OpImplType op_impl_type = EN_IMPL_HW_TBE;
  for(FEOpsStoreInfo &fe_op_store_info: Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_) {
    if (op_impl_type == fe_op_store_info.op_impl_type) {
     fe_op_store_info.need_pre_compile = false;
    }
  }
  bool sgt_flag = false;
  Status status = op_compiler_ptr->PreCompileThreadOp(*graph_, sgt_flag);
  for(FEOpsStoreInfo &fe_op_store_info: Configuration::Instance(AI_CORE_NAME).ops_store_info_vector_) {
    if (op_impl_type == fe_op_store_info.op_impl_type) {
      fe_op_store_info.need_pre_compile = true;
    }
  }
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_op_compiler, case_parse_tvm_json_to_set_attr_fail)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  string json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reductionLayer_1_10_float16__1_SUMSQ_1_with_so.json";
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_INT8);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  op_desc->AddInputDesc("input", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  OpDescPtr op_desc_ptr = node->GetOpDesc();
  Status status = op_compiler_ptr->ParseTvmJsonToSetAttr(node, op_desc_ptr, json_file_path);
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(STEST_fusion_engine_op_compiler, parse_sgt_tvm_json_to_set_attr)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  string json_file_path = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reductionLayer_1_10_float16__1_SUMSQ_1_with_so.json";
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_INT8);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  op_desc->AddInputDesc("input", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  OpDescPtr op_desc_ptr = node->GetOpDesc();
  Status status = op_compiler_ptr->ParseSgtTvmJsonToSetAttr(node, op_desc_ptr, json_file_path);
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(STEST_fusion_engine_op_compiler, parse_json_and_update_op_fail1)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_INT8);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  op_desc->AddInputDesc("input", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  OpDescPtr op_desc_ptr = node->GetOpDesc();
  std::map<int64_t, std::string> scope_json_map;
  scope_json_map[1] = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.json";
  ScopeJsonMap::iterator json_iter = scope_json_map.find(1);
  Status status = op_compiler_ptr->ParseJsonAndUpdateOp(node, op_desc_ptr, json_iter);
  EXPECT_EQ(fe::SUCCESS, status);
}

TEST_F(STEST_fusion_engine_op_compiler, parse_json_and_update_op_fail2)
{
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_INT8);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  op_desc->AddInputDesc("input", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  OpDescPtr op_desc_ptr = node->GetOpDesc();
  std::map<int64_t, std::string> scope_json_map;
  scope_json_map[1] = "air/test/engines/nneng/ut/testcase/fusion_engine/op_compiler/json/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.json";
  ScopeJsonMap::iterator json_iter = scope_json_map.find(1);
  ffts::ThreadSliceMap thread_slice_map;
  thread_slice_map.thread_scope_id = 1;
  ffts::ThreadSliceMapPtr tsmp_ptr = make_shared<ffts::ThreadSliceMap>(thread_slice_map);;
  op_desc_ptr->SetExtAttr("_sgt_json_info", tsmp_ptr);
  Status status = op_compiler_ptr->ParseJsonAndUpdateOp(node, op_desc_ptr, json_iter);
  EXPECT_EQ(fe::SUCCESS, status);
}

const OpStoreAdapterManagerPtr& GetOpStoreAdapterManageStub()
{
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr = std::make_shared<OpStoreAdapterManager>();
  return op_store_adapter_manager_ptr;
}

Status GetOpStoreAdapterStub(OpStoreAdapterManager *This, const OpImplType &op_impl_type, OpStoreAdapterPtr &adapter_ptr)
{
  adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  return fe::SUCCESS;
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_op_success) {
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->Initialize();
  EXPECT_EQ(fe::SUCCESS, status);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  auto op_type = "ProposalD";
  OpDescPtr op_desc1 = std::make_shared<OpDesc>("test1", op_type);
  OpDescPtr op_desc2 = std::make_shared<OpDesc>("test2", op_type);
  // add descriptor
  vector<int64_t> dim(4, 1);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  out_desc.SetOriginFormat(FORMAT_NCHW);
  out_desc.SetFormat(FORMAT_NCHW);
  out_desc.SetDataType(DT_FLOAT16);
  op_desc1->AddInputDesc("cls_prob", out_desc);
  op_desc1->AddInputDesc("bbox_delta", out_desc);
  op_desc1->AddInputDesc("im_info", out_desc);
  op_desc1->AddInputDesc("rpn_bbox", out_desc);
  op_desc1->AddOutputDesc("rois", out_desc);
  op_desc1->AddOutputDesc("actual_rois_num", out_desc);

  op_desc2->AddInputDesc("cls_prob", out_desc);
  op_desc2->AddInputDesc("bbox_delta", out_desc);
  op_desc2->AddInputDesc("im_info", out_desc);
  op_desc2->AddInputDesc("rpn_bbox", out_desc);
  op_desc2->AddOutputDesc("rois", out_desc);
  auto proposal_node1 = graph->AddNode(op_desc1);
  auto proposal_node2 = graph->AddNode(op_desc2);

  OpKernelInfoPtr op_kernel_info = std::shared_ptr<OpKernelInfo>(new (std::nothrow) OpKernelInfo(op_type));
  InputOrOutputInfoPtr output_info_ptr = std::make_shared<InputOrOutputInfo>("rois");
  output_info_ptr->op_param_type_ = OpParamType::REQUIRED;
  op_kernel_info->output_infos_.push_back(output_info_ptr);
  InputOrOutputInfoPtr output_info_ptr1 = std::make_shared<InputOrOutputInfo>("actual_rois_num");
  output_info_ptr1->op_param_type_ = OpParamType::OPTIONAL;
  op_kernel_info->output_infos_.push_back(output_info_ptr1);
  Status res = op_compiler_ptr->SetMemoryTypeForOutput(proposal_node1, op_kernel_info);
  EXPECT_EQ(fe::SUCCESS, res);
  res = op_compiler_ptr->SetMemoryTypeForOutput(proposal_node2, op_kernel_info);
  EXPECT_EQ(fe::SUCCESS, res);

  for (const auto &node : graph->GetDirectNode()) {
    for (int i = 0; i != node->GetOpDesc()->GetAllOutputsDesc().size(); ++i) {
      bool res = IsMemoryEmpty(op_desc1->GetOutputDesc(i));
      if (i == 1) {
        EXPECT_EQ(true, res);
      } else {
        EXPECT_EQ(false, res);
      }
    }
    for (const auto &tensor_desc : node->GetOpDesc()->GetAllInputsDesc()) {
      EXPECT_EQ(false, IsMemoryEmpty(tensor_desc));
    }
  }
}

TEST_F(STEST_fusion_engine_op_compiler, ParseJsonAndCompressOp_success_1) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  NodePtr node_0 = graph->AddNode(op_desc_0);

  map<int64_t, std::string> scope_json_map;
  std::vector<ge::NodePtr> nodes_be_compiled;
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->ParseJsonAndCompressOp(*graph, scope_json_map, nodes_be_compiled);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, ParseJsonAndCompressOp_success_2) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  ge::AttrUtils::SetInt(op_desc_0, SCOPE_ID_ATTR, -4);
  NodePtr node_0 = graph->AddNode(op_desc_0);

  map<string, string> options;
  options.emplace(ge::BUILD_STEP, ge::BUILD_STEP_AFTER_MERGE);
  ge::GetThreadLocalContext().SetGraphOption(options);
  map<int64_t, std::string> scope_json_map;
  std::vector<ge::NodePtr> nodes_be_compiled;
  nodes_be_compiled.push_back(node_0);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->ParseJsonAndCompressOp(*graph, scope_json_map, nodes_be_compiled);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, ParseJsonAndCompressOp_failed) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("relu", "Relu");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  ge::AttrUtils::SetInt(op_desc_0, SCOPE_ID_ATTR, 4);
  NodePtr node_0 = graph->AddNode(op_desc_0);

  map<string, string> options;
  options.emplace(ge::BUILD_STEP, ge::BUILD_STEP_AFTER_MERGE);
  ge::GetThreadLocalContext().SetGraphOption(options);
  map<int64_t, std::string> scope_json_map;
  std::vector<ge::NodePtr> nodes_be_compiled;
  scope_json_map.insert(pair<int64_t, std::string>(4, "air/test/engines/nneng/st/testcase/op_compiler/json/cce_reductionLayer_1_10_float16__1_SUMSQ_1_0.json"));
  nodes_be_compiled.push_back(node_0);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->ParseJsonAndCompressOp(*graph, scope_json_map, nodes_be_compiled);
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_fusion_engine_op_compiler, setcompressweightattr_03)
{
  vector<int64_t> dim1 = {1, 64, 56, 56};
  GeShape shape1(dim1);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_INT8);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  vector<int64_t> dim2 = {256, 64, 1, 1};
  GeShape shape2(dim2);
  GeTensorDesc tensor_desc2(shape2);
  tensor_desc2.SetOriginDataType(DT_INT8);
  tensor_desc2.SetDataType(DT_INT8);
  tensor_desc2.SetFormat(FORMAT_NCHW);
  tensor_desc2.SetOriginFormat(FORMAT_NCHW);
  tensor_desc2.SetOriginShape(shape2);

  vector<int64_t> dim3 = {256};
  GeShape shape3(dim3);
  GeTensorDesc tensor_desc3(shape3);
  tensor_desc3.SetOriginDataType(DT_INT8);
  tensor_desc3.SetDataType(DT_INT8);
  tensor_desc3.SetFormat(FORMAT_NCHW);
  tensor_desc3.SetOriginFormat(FORMAT_NCHW);
  tensor_desc3.SetOriginShape(shape3);

  vector<int64_t> dim4 = {1, 256, 56, 56};
  GeShape shape4(dim4);
  GeTensorDesc tensor_desc4(shape4);
  tensor_desc4.SetOriginDataType(DT_INT8);
  tensor_desc4.SetDataType(DT_INT8);
  tensor_desc4.SetFormat(FORMAT_NCHW);
  tensor_desc4.SetOriginFormat(FORMAT_NCHW);
  tensor_desc4.SetOriginShape(shape4);

  OpDescPtr op_desc = std::make_shared<OpDesc>("conv2d2", "Conv2D");
  op_desc->AddInputDesc("input", tensor_desc1);
  op_desc->AddInputDesc("filter", tensor_desc2);
  op_desc->AddInputDesc("bias", tensor_desc3);
  op_desc->AddOutputDesc("out", tensor_desc4);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  string path = "./air/test/engines/nneng/config/data/platform_config";
  string real_path = RealPath(path);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
  PlatformInfoManager::Instance().LoadConfigFile(real_path);
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Hi3796CV300ES";

  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->SetCompressWeightAttr(node);
  EXPECT_EQ(fe::SUCCESS, status);
  bool has_fe_weight_attr = ge::AttrUtils::HasAttr(op_desc, ATTR_NAME_FE_WEIGHT_COMPRESS);
  EXPECT_EQ(has_fe_weight_attr, false);
  PlatformInfoManager::Instance().platform_info_map_.clear();
  PlatformInfoManager::Instance().platform_infos_map_.clear();
}

TEST_F(STEST_fusion_engine_op_compiler, setcompressweightattr_04)
{
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_INT8);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("fc", "FullyConnection");
  op_desc->AddInputDesc("input", tensor_desc1);
  op_desc->AddInputDesc("filter", tensor_desc1);
  op_desc->AddInputDesc("bias", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->SetCompressWeightAttr(node);
  EXPECT_EQ(fe::SUCCESS, status);
  bool has_fe_weight_attr = ge::AttrUtils::HasAttr(op_desc, ATTR_NAME_FE_WEIGHT_COMPRESS);
  EXPECT_EQ(has_fe_weight_attr, true);
  bool fe_weight_compress = false;
  ge::AttrUtils::GetBool(op_desc, ATTR_NAME_FE_WEIGHT_COMPRESS, fe_weight_compress);
  EXPECT_EQ(fe_weight_compress, true);
}

TEST_F(STEST_fusion_engine_op_compiler, setcompressweightattr_05)
{
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_INT8);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("relu", "Relu");
  op_desc->AddInputDesc("input", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->SetCompressWeightAttr(node);
  EXPECT_EQ(fe::SUCCESS, status);
  bool has_fe_weight_attr = ge::AttrUtils::HasAttr(op_desc, ATTR_NAME_FE_WEIGHT_COMPRESS);
  EXPECT_EQ(has_fe_weight_attr, false);
}

TEST_F(STEST_fusion_engine_op_compiler, setcompressweightattr_06)
{
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_UNDEFINED);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("MatMulV2", "MatMulV2");
  op_desc->AddInputDesc("x", tensor_desc1);
  op_desc->AddInputDesc("y", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->SetCompressWeightAttr(node);
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(STEST_fusion_engine_op_compiler, setcompressweightattr_07)
{
  vector<int64_t> dim(4, 2);
  GeShape shape1(dim);
  GeTensorDesc tensor_desc1(shape1);
  tensor_desc1.SetOriginDataType(DT_MAX);
  tensor_desc1.SetDataType(DT_INT8);
  tensor_desc1.SetFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginFormat(FORMAT_NCHW);
  tensor_desc1.SetOriginShape(shape1);

  OpDescPtr op_desc = std::make_shared<OpDesc>("MatMulV2", "MatMulV2");
  op_desc->AddInputDesc("x", tensor_desc1);
  op_desc->AddInputDesc("y", tensor_desc1);
  op_desc->AddOutputDesc("out", tensor_desc1);

  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc);

  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->SetCompressWeightAttr(node);
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(STEST_fusion_engine_op_compiler, getfusionscope_1)
{
  ComputeGraphPtr graph = BuildTestGraph(1);
  ScopeNodeIdMap fusion_nodes_map;
  vector<ge::NodePtr> nodes_be_compiled;
  std::vector<ge::NodePtr> buff_fus_rollback_nodes;
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->GetFusionScope(*graph, buff_fus_rollback_nodes, fusion_nodes_map, nodes_be_compiled);
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto iter = fusion_nodes_map.begin(); iter != fusion_nodes_map.end(); iter++) {
    if (iter->first == 3) {
      EXPECT_EQ(iter->second.size(), 4);
    }
  }
}

TEST_F(STEST_fusion_engine_op_compiler, getfusionscope_2)
{
  ComputeGraphPtr graph = BuildTestGraph(2);
  ScopeNodeIdMap fusion_nodes_map;
  vector<ge::NodePtr> nodes_be_compiled;
  std::vector<ge::NodePtr> buff_fus_rollback_nodes;
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->GetFusionScope(*graph, buff_fus_rollback_nodes, fusion_nodes_map, nodes_be_compiled);
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto iter = fusion_nodes_map.begin(); iter != fusion_nodes_map.end(); iter++) {
    if (iter->first == 1 || iter->first == 2) {
      EXPECT_EQ(iter->second.size(), 2);
    }
  }
}

TEST_F(STEST_fusion_engine_op_compiler, getfusionscope_3)
{
  ComputeGraphPtr graph = BuildTestGraph(3);
  ScopeNodeIdMap fusion_nodes_map;
  vector<ge::NodePtr> nodes_be_compiled;
  std::vector<ge::NodePtr> buff_fus_rollback_nodes;
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->GetFusionScope(*graph, buff_fus_rollback_nodes, fusion_nodes_map, nodes_be_compiled);
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto iter = fusion_nodes_map.begin(); iter != fusion_nodes_map.end(); iter++) {
    if (iter->first == 2 || iter->first == 3) {
      EXPECT_EQ(iter->second.size(), 2);
    }
  }
}

TEST_F(STEST_fusion_engine_op_compiler, getfusionscope_4)
{
  ComputeGraphPtr graph = BuildTestGraph(4);
  ScopeNodeIdMap fusion_nodes_map;
  vector<ge::NodePtr> nodes_be_compiled;
  std::vector<ge::NodePtr> buff_fus_rollback_nodes;
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->GetFusionScope(*graph, buff_fus_rollback_nodes, fusion_nodes_map, nodes_be_compiled);
  EXPECT_EQ(fe::SUCCESS, status);
  for (auto iter = fusion_nodes_map.begin(); iter != fusion_nodes_map.end(); iter++) {
    if (iter->first == 1 || iter->first == 3) {
      EXPECT_EQ(iter->second.size(), 2);
    }
  }
}

TEST_F(STEST_fusion_engine_op_compiler, getfusionscope_5)
{
  ComputeGraphPtr graph = BuildTestGraph(5);
  ScopeNodeIdMap fusion_nodes_map;
  vector<ge::NodePtr> nodes_be_compiled;
  std::vector<ge::NodePtr> buff_fus_rollback_nodes;
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status status = op_compiler_ptr->GetFusionScope(*graph, buff_fus_rollback_nodes, fusion_nodes_map, nodes_be_compiled);
  EXPECT_EQ(fe::FAILED, status);
}

TEST_F(STEST_fusion_engine_op_compiler, init_and_finalize_1)
{
  std::string te_so_path = "air/build/test/engines/nneng/depends/te_fusion/";
  std::string real_path = GetRealPath(te_so_path);
  real_path +="/";
  Configuration::Instance(AI_CORE_NAME).content_map_.emplace("rootdir", real_path);
  Configuration::Instance(AI_CORE_NAME).op_select_impl_mode_map_.emplace("Relu", "high_performance");
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  Status ret = tbe_adapter_ptr->Finalize();
  EXPECT_EQ(ret, fe::FAILED);
  std::map<std::string, std::string> options;
  options.emplace("ge.socVersion", "Ascend310");
  ret = tbe_adapter_ptr->Initialize(options, AI_CORE_NAME);
  EXPECT_EQ(ret, fe::SUCCESS);
  ret = tbe_adapter_ptr->Initialize(options, AI_CORE_NAME);
  EXPECT_EQ(ret, fe::SUCCESS);
  ret = tbe_adapter_ptr->Finalize();
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, change_buffer_optimize_1)
{
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  std::map<std::string, std::string> options;
  options.emplace("ge.bufferOptimize", "l1_optimize");
  std::map<std::string, std::string> new_options;
  tbe_adapter_ptr->ChangeBufferOptimize(options, new_options);
  options["ge.bufferOptimize"] = "l2_optimize";
  tbe_adapter_ptr->ChangeBufferOptimize(options, new_options);
}

TEST_F(STEST_fusion_engine_op_compiler, init_tbe_func)
{
  std::string tbe_so_path = "air/build/test/engines/nneng/depends/te_fusion/libte_fusion_stub.so";
  std::string real_path = GetRealPath(tbe_so_path);
  PluginManagerPtr plugin_manager_ptr = std::make_shared<PluginManager>(tbe_so_path);
  plugin_manager_ptr->OpenPlugin(real_path);
  TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
  tbe_adapter_ptr->StopCompileOpInTuningAndAfterUBMatchMode();
  Status ret = tbe_adapter_ptr->InitTbeFunctions(plugin_manager_ptr);
  EXPECT_EQ(ret, fe::SUCCESS);
  plugin_manager_ptr->CloseHandle();
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_case_1)
{
  tbe_adapter_ptr_->support_parallel_compile = false;
  ComputeGraphPtr graph = BuildSomeGraph(false, 0);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->PreCompileOp(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_case_2)
{
  tbe_adapter_ptr_->support_parallel_compile = true;
  ComputeGraphPtr graph = BuildSomeGraph(false, 0);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->PreCompileOp(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_case_3)
{
  tbe_adapter_ptr_->support_parallel_compile = true;
  ComputeGraphPtr graph = BuildSomeGraph(true, 0);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->PreCompileOp(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_case_4)
{
  tbe_adapter_ptr_->support_parallel_compile = true;
  ComputeGraphPtr graph = BuildSomeGraph(false, 1);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->PreCompileOp(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_case_5)
{
  tbe_adapter_ptr_->support_parallel_compile = true;
  ComputeGraphPtr graph = BuildSomeGraph(false, 2);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->PreCompileOp(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_case_6)
{
  tbe_adapter_ptr_->support_parallel_compile = true;
  ComputeGraphPtr graph = BuildSomeGraph(false, 3);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->PreCompileOp(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, pre_compile_case_7)
{
  tbe_adapter_ptr_->support_parallel_compile = true;
  ComputeGraphPtr graph = BuildSomeGraph(true, 4);
  auto op_compiler_ptr = std::make_shared<OpCompiler>("normal compiler", AI_CORE_NAME, op_store_adapter_manager_ptr_);
  Status ret = op_compiler_ptr->PreCompileOp(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(STEST_fusion_engine_op_compiler, generate_lxfusion_optimize_result_1) {
  Status l1_buffer_ret = tune::SUCCESS;
  Status l2_buffer_ret = tune::HIT_FUSION_STRATEGY;
  auto fe_graph_optimizer_ptr = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  LxFusionOptimizeResult opt_ret = fe_graph_optimizer_ptr->GenerateLxFusionOptimizeResult(tune::SUCCESS, tune::HIT_FUSION_STRATEGY);
  EXPECT_EQ(opt_ret, LxFusionOptimizeResult::BOTH_FUSION_STRATEGY);

  opt_ret = fe_graph_optimizer_ptr->GenerateLxFusionOptimizeResult(tune::HIT_FUSION_STRATEGY, tune::SUCCESS);
  EXPECT_EQ(opt_ret, LxFusionOptimizeResult::BOTH_FUSION_STRATEGY);

  opt_ret = fe_graph_optimizer_ptr->GenerateLxFusionOptimizeResult(tune::SUCCESS, tune::NO_FUSION_STRATEGY);
  EXPECT_EQ(opt_ret, LxFusionOptimizeResult::L1_FUSION_STRATEGY);

  opt_ret = fe_graph_optimizer_ptr->GenerateLxFusionOptimizeResult(tune::NO_FUSION_STRATEGY, tune::SUCCESS);
  EXPECT_EQ(opt_ret, LxFusionOptimizeResult::L2_FUSION_STRATEGY);

  opt_ret = fe_graph_optimizer_ptr->GenerateLxFusionOptimizeResult(tune::NO_FUSION_STRATEGY, tune::NO_FUSION_STRATEGY);
  EXPECT_EQ(opt_ret, LxFusionOptimizeResult::NO_FUSION_STRATEGY);
}

Status L1FusionOptimizerStub(ge::ComputeGraph &graph, GraphCommPtr graphCommPtr, ScopeAllocatorPtr scopeAllocatorPtr,
                             const std::string &engineName, const fe::AOEOption options) {
  return fe::SUCCESS;
}

Status L2FusionOptimizerStub(ge::ComputeGraph &graph, GraphCommPtr graphCommPtr, ScopeAllocatorPtr scopeAllocatorPtr,
                             const std::string &engineName, const fe::AOEOption options) {
  return fe::SUCCESS;
}

TEST_F(STEST_fusion_engine_op_compiler, buffer_fusion_process_case_1) {
  Configuration::Instance(AI_CORE_NAME).buffer_optimize_ = EN_L1_OPTIMIZE;
  Configuration::Instance(AI_CORE_NAME).buffer_fusion_mode_ = EN_L2_FUSION;
  Configuration::Instance(AI_CORE_NAME).soc_version_ = "Ascend710";
  std::shared_ptr<fe::FEGraphOptimizer> fe_graph_optimizer_ptr
          = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L1FusionOptimizer = L1FusionOptimizerStub;
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L2FusionOptimizer = L2FusionOptimizerStub;

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(AI_CORE_NAME);
  LxFusionOptimizeResult buffer_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Status status = fe_graph_optimizer_ptr->BufferFusionProcess(*graph, graph_comm_ptr, buffer_ret);
  EXPECT_EQ(status, fe::SUCCESS);
  EXPECT_EQ(buffer_ret, LxFusionOptimizeResult::BOTH_FUSION_STRATEGY);
}

TEST_F(STEST_fusion_engine_op_compiler, buffer_fusion_process_case_2) {
  Configuration::Instance(AI_CORE_NAME).buffer_optimize_ = EN_L1_OPTIMIZE;
  Configuration::Instance(AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  std::shared_ptr<fe::FEGraphOptimizer> fe_graph_optimizer_ptr
          = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L1FusionOptimizer = L1FusionOptimizerStub;
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L2FusionOptimizer = L2FusionOptimizerStub;

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(AI_CORE_NAME);
  LxFusionOptimizeResult buffer_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Status status = fe_graph_optimizer_ptr->BufferFusionProcess(*graph, graph_comm_ptr, buffer_ret);
  EXPECT_EQ(status, fe::SUCCESS);
  EXPECT_EQ(buffer_ret, LxFusionOptimizeResult::L1_FUSION_STRATEGY);
}

TEST_F(STEST_fusion_engine_op_compiler, buffer_fusion_process_case_3) {
  Configuration::Instance(AI_CORE_NAME).buffer_optimize_ = EN_OFF_OPTIMIZE;
  Configuration::Instance(AI_CORE_NAME).buffer_fusion_mode_ = EN_L2_FUSION;
  std::shared_ptr<fe::FEGraphOptimizer> fe_graph_optimizer_ptr
          = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L1FusionOptimizer = L1FusionOptimizerStub;
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L2FusionOptimizer = L2FusionOptimizerStub;

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(AI_CORE_NAME);
  LxFusionOptimizeResult buffer_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Status status = fe_graph_optimizer_ptr->BufferFusionProcess(*graph, graph_comm_ptr, buffer_ret);
  EXPECT_EQ(status, fe::SUCCESS);
  EXPECT_EQ(buffer_ret, LxFusionOptimizeResult::L2_FUSION_STRATEGY);
}

TEST_F(STEST_fusion_engine_op_compiler, buffer_fusion_process_case_4) {
  Configuration::Instance(AI_CORE_NAME).buffer_optimize_ = EN_OFF_OPTIMIZE;
  Configuration::Instance(AI_CORE_NAME).buffer_fusion_mode_ = EN_OPTIMIZE_DISABLE;
  std::shared_ptr<fe::FEGraphOptimizer> fe_graph_optimizer_ptr
          = std::make_shared<FEGraphOptimizer>(ops_kernel_info_store_ptr_, op_store_adapter_manager_ptr_, AI_CORE_NAME);
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L1FusionOptimizer = L1FusionOptimizerStub;
  fe_graph_optimizer_ptr->fusion_pass_mgr_ptr_->L2FusionOptimizer = L2FusionOptimizerStub;

  std::shared_ptr<GraphComm> graph_comm_ptr = std::make_shared<GraphComm>(AI_CORE_NAME);
  LxFusionOptimizeResult buffer_ret = LxFusionOptimizeResult::NO_FUSION_STRATEGY;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  Status status = fe_graph_optimizer_ptr->BufferFusionProcess(*graph, graph_comm_ptr, buffer_ret);
  EXPECT_EQ(status, fe::SUCCESS);
  EXPECT_EQ(buffer_ret, LxFusionOptimizeResult::NO_FUSION_STRATEGY);
}