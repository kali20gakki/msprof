/**
 * Copyright 2020-2021 Huawei Technologies Co., Ltd
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

#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/stride_hoist_pass.h"
#include "common/pass_manager.h"
#include "graph/compute_graph.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#include "graph_optimizer/graph_fusion/graph_fusion.h"
#include "common/configuration.h"
#include "adapter/tbe_adapter/tbe_op_store_adapter.h"
#include "ops_store/ops_kernel_manager.h"
#include "register/graph_optimizer/graph_fusion/fusion_pass_manager/fusion_pass_registry.h"
#include "register/graph_optimizer/buffer_fusion/buffer_fusion_pass_registry.h"
#include "common/constants_define.h"
#include "graph_optimizer/fusion_common/graph_node_map_util.h"
#include "graph_optimizer/fe_graph_optimizer.h"
#undef protected
#undef private

using namespace testing;
using namespace ge;
using namespace fe;
using namespace std;

namespace fe {

using FEGraphOptimizerPtr = std::shared_ptr<FEGraphOptimizer>;
class GRAPH_FUSION_ST : public testing::Test {
 public:

 protected:

  void SetUp() {
    op_store_adapter_manager_ = std::make_shared<OpStoreAdapterManager>();
    TbeOpStoreAdapterPtr tbe_adapter_ptr = std::make_shared<TbeOpStoreAdapter>();
    op_store_adapter_manager_->map_all_op_store_adapter_.emplace(
        std::make_pair("tbe_op_adapter", tbe_adapter_ptr));
    std::map<std::string, std::string> options;
    fe_ops_kernel_info_store_ = make_shared<fe::FEOpsKernelInfoStore>(
        op_store_adapter_manager_);
    FEOpsStoreInfo heavy_op_info{
        6,
        "tbe-builtin",
        EN_IMPL_HW_TBE,
        "./air/test/engines/nneng/ut/testcase/fusion_engine/ops_kernel_store/fe_config/fusion_rule_manager",
        "",
        false,
        false};

    vector<FEOpsStoreInfo> store_info;
    store_info.emplace_back(heavy_op_info);
    Configuration::Instance(fe::AI_CORE_NAME).ops_store_info_vector_ = (store_info);
    OpsKernelManager::Instance(AI_CORE_NAME).Finalize();
    // initialize fusion rules
    string file_path =
        "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_rule_parser/cycle_detection.json";
    fe_ops_kernel_info_store_->Initialize(options);
    fusion_rule_mgr_ = std::make_shared<FusionRuleManager>(fe_ops_kernel_info_store_);
    Configuration::Instance(AI_CORE_NAME).ascend_ops_path_ = "";
    Configuration::Instance(AI_CORE_NAME).content_map_[custom_path_key_] = file_path;
    Configuration::Instance(AI_CORE_NAME).content_map_[built_in_path_key_] = file_path;
    Configuration::Instance(AI_CORE_NAME).content_map_[FUSION_CONFIG_BUILT_IN_FILE] =
        "plugin/opskernel/fe_config/fusion_config.json";
    fusion_rule_mgr_->Initialize(AI_CORE_NAME);

    fusion_pass_mgr_ = std::make_shared<FusionPassManager>();
    fusion_priority_mgr_ =
        std::make_shared<FusionPriorityManager>(AI_CORE_NAME, fusion_pass_mgr_, fusion_rule_mgr_);

    fusion_priority_mgr_vec_ =
        std::make_shared<FusionPriorityManager>(VECTOR_CORE_NAME, fusion_pass_mgr_, fusion_rule_mgr_);

    // initialize fusion configuration
    Configuration::Instance(fe::AI_CORE_NAME).lib_path_ =
        "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config3/";
    Configuration::Instance(fe::AI_CORE_NAME).custom_fusion_config_file_ =
        "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config3.json";
    fusion_priority_mgr_->fusion_config_parser_ptr_ = std::make_shared<FusionConfigParser>(fe::AI_CORE_NAME);
    fusion_priority_mgr_->fusion_config_parser_ptr_->ParseFusionConfigFile();

    Configuration::Instance(fe::VECTOR_CORE_NAME).lib_path_ =
        "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config3/";
    Configuration::Instance(fe::VECTOR_CORE_NAME).custom_fusion_config_file_ =
        "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config3.json";
    fusion_priority_mgr_vec_->fusion_config_parser_ptr_ = std::make_shared<FusionConfigParser>(fe::VECTOR_CORE_NAME);
    fusion_priority_mgr_vec_->fusion_config_parser_ptr_->ParseFusionConfigFile();

    graph_fusion_ = std::make_shared<GraphFusion>(
        fusion_rule_mgr_, fe_ops_kernel_info_store_,
        fusion_pass_mgr_, fusion_priority_mgr_);
    graph_fusion_->SetEngineName(AI_CORE_NAME);

    graph_fusion_vec_ = std::make_shared<GraphFusion>(
        fusion_rule_mgr_, fe_ops_kernel_info_store_,
        fusion_pass_mgr_, fusion_priority_mgr_vec_);
    graph_fusion_vec_->SetEngineName(VECTOR_CORE_NAME);

    graph_optimizer_ = std::make_shared<FEGraphOptimizer>(fe_ops_kernel_info_store_,
                                                          op_store_adapter_manager_);
    graph_optimizer_->init_flag_ = true;
    graph_optimizer_->graph_fusion_ptr_ = graph_fusion_;
    graph_optimizer_->fusion_pass_mgr_ptr_ = fusion_pass_mgr_;
    graph_optimizer_->fusion_priority_mgr_ptr_ = fusion_priority_mgr_;
    graph_optimizer_->fusion_rule_mgr_ptr_ = fusion_rule_mgr_;
  }

  void TearDown() {
  }

  shared_ptr<fe::FEOpsKernelInfoStore> fe_ops_kernel_info_store_;
  OpStoreAdapterManagerPtr op_store_adapter_manager_;
  FusionRuleManagerPtr fusion_rule_mgr_;
  FusionPassMgrPtr fusion_pass_mgr_;
  FusionPriorityMgrPtr fusion_priority_mgr_;
  FusionPriorityMgrPtr fusion_priority_mgr_vec_;
  FEGraphOptimizerPtr graph_optimizer_;
  shared_ptr<GraphFusion> graph_fusion_;
  shared_ptr<GraphFusion> graph_fusion_vec_;
  string ori_path_;
  string ori_opp_path_;
  const string custom_path_key_ = "fusionrulemgr.aicore.customfilepath";
  const string built_in_path_key_ = "fusionrulemgr.aicore.graphfilepath";
};

TEST_F(GRAPH_FUSION_ST, converage_03) {
  int32_t priority = CUSTOM_CFG_DOWN_PRIORITY_MIN;
  fusion_priority_mgr_->AdjustDownStagePriority(priority);
  fusion_priority_mgr_->GetRealPriority(RESERVED_FOR_DOWN_PRIORITY + 1);
}

class TestPass : public PatternFusionBasePass {
 protected:

  vector<FusionPattern *> DefinePatterns() override {
    return {};
  };

  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override {
    return SUCCESS;
  }
};

class TestFailedPass : public PatternFusionBasePass {
 protected:

  vector<FusionPattern *> DefinePatterns() override {
    vector<FusionPattern *> patterns;
    FusionPattern *pattern = new (std::nothrow) FusionPattern("FailedPattern");
    FE_CHECK(pattern == nullptr, REPORT_FE_ERROR("[GraphOpt][ConCatQuatFus][DfnPtn] Fail to new an object."),
             return patterns);

    pattern->AddOpDesc("pattern_dequant", {ASCEND_DEQUANT})
        .SetOutput("pattern_dequant");
    patterns.push_back(pattern);

    return patterns;
  };

  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &fusion_nodes) override {
    return FAILED;
  }
};

using CreateFn = GraphPass *(*)();

fe::GraphPass *CreateFunc() {
  return new(std::nothrow) TestPass();
}

fe::GraphPass *CreateFailedFunc() {
  return new(std::nothrow) TestFailedPass();

}

void RegisterPassFunc(CreateFn create_fn) {
  FusionPassRegistry::GetInstance().RegisterPass(CUSTOM_AI_CORE_GRAPH_PASS, "CUSTOM_PASS1", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(CUSTOM_AI_CORE_GRAPH_PASS, "CUSTOM_PASS2", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(CUSTOM_AI_CORE_GRAPH_PASS, "CUSTOM_PASS3", create_fn);

  FusionPassRegistry::GetInstance().RegisterPass(BUILT_IN_GRAPH_PASS, "BUILT_IN_PASS1", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(BUILT_IN_GRAPH_PASS, "BUILT_IN_PASS2", create_fn);

  FusionPassRegistry::GetInstance().RegisterPass(SECOND_ROUND_BUILT_IN_GRAPH_PASS, "BUILT_IN_PASS3", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(SECOND_ROUND_BUILT_IN_GRAPH_PASS, "BUILT_IN_PASS4", create_fn);

  FusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS, "BUILT_IN_PASS3", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS, "BUILT_IN_PASS4", create_fn);

  FusionPassRegistry::GetInstance().RegisterPass(BUILT_IN_PREPARE_GRAPH_PASS, "PREPARE_PASS1", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(BUILT_IN_PREPARE_GRAPH_PASS, "PREPARE_PASS2", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(BUILT_IN_PREPARE_GRAPH_PASS, "PREPARE_PASS3", create_fn);

  FusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS, "BEFORE_QUANT_1", create_fn);
  FusionPassRegistry::GetInstance().RegisterPass(
      BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS, "BEFORE_QUANT_2", create_fn);
}

TEST_F(GRAPH_FUSION_ST, converage_04) {
  RegisterPassFunc(CreateFunc);
  EXPECT_EQ(SUCCESS, fusion_priority_mgr_->SortGraphFusion());
  EXPECT_EQ(SUCCESS, fusion_priority_mgr_->SortGraphFusion());
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  Configuration::Instance(AI_CORE_NAME).enable_network_analysis_ = true;
  EXPECT_EQ(SUCCESS, graph_fusion_->FusionEachGraph(*graph));
  string stage = "test";
  EXPECT_EQ(SUCCESS, graph_fusion_->RunGraphFusionPassByType(stage, *graph, SECOND_ROUND_BUILT_IN_GRAPH_PASS));
  EXPECT_EQ(SUCCESS, graph_fusion_->RunGraphFusionPassByType(stage, *graph,
                                                             BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS));
  EXPECT_EQ(SUCCESS, graph_fusion_->FusionQuantOp(*graph));
  Configuration::Instance(AI_CORE_NAME).enable_network_analysis_ = false;
}

ge::NodePtr AddOneNode(ge::ComputeGraphPtr &graph,
                       string node_name, const string &node_type) {
  ge::OpDescPtr op = std::make_shared<ge::OpDesc>(node_name, node_type);
  ge::GeTensorDesc tensor_desc(ge::GeShape({10}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  op->AddInputDesc(tensor_desc);
  op->AddOutputDesc(tensor_desc);
  return graph->AddNode(op);
}

TEST_F(GRAPH_FUSION_ST, converage_05) {
  RegisterPassFunc(CreateFailedFunc);

  EXPECT_EQ(SUCCESS, fusion_priority_mgr_->SortGraphFusion());

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");


  auto node1 = AddOneNode(graph, "dequant1", ASCEND_DEQUANT);
  auto node2 = AddOneNode(graph, "dequant2", ASCEND_DEQUANT);
  auto node3 = AddOneNode(graph, "dequant3", ASCEND_DEQUANT);

  ge::GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node2->GetOutDataAnchor(0), node3->GetInDataAnchor(0));
  EXPECT_EQ(FAILED, graph_fusion_->FusionEachGraph(*graph));
  string stage = "a";
  EXPECT_EQ(FAILED, graph_fusion_->RunGraphFusionPassByType(stage, *graph, SECOND_ROUND_BUILT_IN_GRAPH_PASS));
  EXPECT_EQ(FAILED, graph_fusion_->RunGraphFusionPassByType(stage, *graph, BUILT_IN_BEFORE_TRANSNODE_INSERTION_GRAPH_PASS));
  EXPECT_EQ(FAILED, graph_fusion_->RunGraphFusionPassByType(stage, *graph, BUILT_IN_PREPARE_GRAPH_PASS));
  EXPECT_EQ(FAILED, graph_fusion_->RunGraphFusionPassByType(stage, *graph, BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS));
}

TEST_F(GRAPH_FUSION_ST, converage_06) {
  EXPECT_EQ(SUCCESS, fusion_priority_mgr_->SortGraphFusion());

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V100;
  EXPECT_EQ(SUCCESS, graph_fusion_->FusionQuantOp(*graph));
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  EXPECT_EQ(SUCCESS, graph_fusion_->FusionQuantOp(*graph));
}

TEST_F(GRAPH_FUSION_ST, converage_07) {
  RegisterPassFunc(CreateFunc);
  EXPECT_EQ(SUCCESS, fusion_priority_mgr_->SortGraphFusion());
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::GeTensorDesc tensor_desc(ge::GeShape({10}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  ge::OpDescPtr dequant = std::make_shared<ge::OpDesc>("dequant", ASCEND_DEQUANT);
  dequant->AddInputDesc(tensor_desc);
  dequant->AddOutputDesc(tensor_desc);

  ge::OpDescPtr other = std::make_shared<ge::OpDesc>("other", "Other");
  other->AddInputDesc(tensor_desc);
  other->AddOutputDesc(tensor_desc);

  auto dequant_node = graph->AddNode(dequant);
  graph->AddNode(other);

  ge::GeTensorPtr const_out_tenosr = nullptr;
  const_out_tenosr = std::make_shared<ge::GeTensor>(tensor_desc);

  vector<uint64_t> data;
  data.emplace_back(0xFF0120312);

  const_out_tenosr->SetData(reinterpret_cast<uint8_t *>(data.data()),
                            sizeof(uint64_t));

  ge::OpDescPtr const_op_desc = ge::OpDescUtils::CreateConstOp(const_out_tenosr);
  auto const_node = graph->AddNode(const_op_desc);
  ge::OpDescUtils::SetWeights(dequant_node->GetOpDesc(), const_out_tenosr);
  ASSERT_EQ(ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0),
                                    dequant_node->GetInDataAnchor(0)), SUCCESS);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  EXPECT_EQ(SUCCESS, graph_fusion_->JudgeQuantMode(*graph));
}

TEST_F(GRAPH_FUSION_ST, converage_08) {
  RegisterPassFunc(CreateFunc);
  EXPECT_EQ(SUCCESS, fusion_priority_mgr_->SortGraphFusion());
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  ge::GeTensorDesc tensor_desc(ge::GeShape({10, 20}), ge::FORMAT_NCHW, ge::DT_FLOAT);
  ge::OpDescPtr dequant = std::make_shared<ge::OpDesc>("dequant", ASCEND_DEQUANT);
  dequant->AddInputDesc(tensor_desc);
  dequant->AddOutputDesc(tensor_desc);

  ge::OpDescPtr other = std::make_shared<ge::OpDesc>("other", "Other");
  other->AddInputDesc(tensor_desc);
  other->AddOutputDesc(tensor_desc);

  auto dequant_node = graph->AddNode(dequant);
  graph->AddNode(other);

  ge::GeTensorPtr const_out_tenosr = nullptr;
  const_out_tenosr = std::make_shared<ge::GeTensor>(tensor_desc);

  vector<uint64_t> data;
  data.emplace_back(0xFF0120312);

  const_out_tenosr->SetData(reinterpret_cast<uint8_t *>(data.data()),
                            sizeof(uint64_t));

  ge::OpDescPtr const_op_desc = ge::OpDescUtils::CreateConstOp(const_out_tenosr);
  auto const_node = graph->AddNode(const_op_desc);
  ge::OpDescUtils::SetWeights(dequant_node->GetOpDesc(), const_out_tenosr);
  ASSERT_EQ(ge::GraphUtils::AddEdge(const_node->GetOutDataAnchor(0),
                                    dequant_node->GetInDataAnchor(0)), SUCCESS);
  Configuration::Instance(AI_CORE_NAME).isa_arch_ver_ = ISAArchVersion::EN_ISA_ARCH_V200;
  EXPECT_EQ(PARAM_INVALID, graph_fusion_->JudgeQuantMode(*graph));
}

TEST_F(GRAPH_FUSION_ST, converage_09) {
  fusion_pass_mgr_->Initialize(AI_CORE_NAME);
  fusion_pass_mgr_->Finalize();

  fusion_pass_mgr_->Initialize(VECTOR_CORE_NAME);
  fusion_pass_mgr_->Finalize();
}

TEST_F(GRAPH_FUSION_ST, converage_09_1) {
  // config fusion pass
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";

  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";
  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";

  fusion_pass_mgr_->Initialize(AI_CORE_NAME);
  fusion_pass_mgr_->Finalize();
}

TEST_F(GRAPH_FUSION_ST, converage_09_2) {
  // config fusion pass
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";

  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";
  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";

  fusion_pass_mgr_->Initialize(VECTOR_CORE_NAME);
  fusion_pass_mgr_->Finalize();
}

TEST_F(GRAPH_FUSION_ST, converage_09_3) {
  // config fusion pass
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager";
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager";

  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager";
  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager";

  Configuration::Instance(AI_CORE_NAME).lib_path_ =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/l2_optimizer/plugin/opskernel/";
  fusion_pass_mgr_->Initialize(AI_CORE_NAME);
  fusion_pass_mgr_->Finalize();
}

TEST_F(GRAPH_FUSION_ST, converage_09_4) {
  // config fusion pass
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";
  Configuration::Instance(AI_CORE_NAME).content_map_[CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";

  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_CUSTOM_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";
  Configuration::Instance(VECTOR_CORE_NAME).content_map_[VECTOR_CORE_CONFIG_KEY_BUILTIN_PASS_FILE] =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/graph_optimizer/graph_fusion/pass_fusion_manager/libops_fusion_pass_aicore.so";

  Configuration::Instance(VECTOR_CORE_NAME).lib_path_ =
      "./air/test/engines/nneng/ut/testcase/fusion_engine/l2_optimizer/plugin/opskernel/";
  fusion_pass_mgr_->Initialize(VECTOR_CORE_NAME);
  fusion_pass_mgr_->Finalize();
}

TEST_F(GRAPH_FUSION_ST, converage_10) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  fe::GraphNodeMapUtil::ReCreateNodeTypeMapInGraph(*graph);
  ge::OpDescPtr op = std::make_shared<OpDesc>("test_op", "TestOp");
  auto node = graph->AddNode(op);

  std::map<std::string, ge::NodePtr> inner_map;
  inner_map["test"] = node;
  std::unordered_map<std::string, std::map<std::string, ge::NodePtr>> node_map;
  node_map["test"] = inner_map;

  NodeMapInfoPtr info = std::make_shared<NodeMapInfo>();

  NodeTypeMapPtr node_type_map = std::make_shared<NodeTypeMap>(node_map);
  info->node_type_map = node_type_map;
  graph->SetExtAttr("NodeMapInfo", info);
  fe::GraphNodeMapUtil::ReCreateNodeTypeMapInGraph(*graph);
}

TEST_F(GRAPH_FUSION_ST, converage_11) {
  RegisterPassFunc(CreateFunc);
  EXPECT_EQ(SUCCESS, fusion_priority_mgr_->SortGraphFusion());

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  Configuration::Instance(AI_CORE_NAME).enable_network_analysis_ = true;
  EXPECT_EQ(SUCCESS, graph_fusion_->FusionEachGraph(*graph));
  string stage = "test";
  EXPECT_EQ(SUCCESS, graph_fusion_->RunGraphFusionPassByType(stage, *graph, BUILT_IN_PREPARE_GRAPH_PASS));
  EXPECT_EQ(SUCCESS, graph_fusion_->RunGraphFusionPassByType(stage, *graph, BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS));
  EXPECT_EQ(SUCCESS, graph_fusion_->FusionQuantOp(*graph));
  Configuration::Instance(AI_CORE_NAME).enable_network_analysis_ = false;

  vector<FusionPassOrRule> graph_fusion_pass_vector;
  fusion_priority_mgr_->GetGraphFusionPassInfosByType(BUILT_IN_PREPARE_GRAPH_PASS, graph_fusion_pass_vector);
  fusion_priority_mgr_->GetGraphFusionPassInfosByType(BUILT_IN_BEFORE_QUANT_OPTIMIZATION_GRAPH_PASS,
                                                      graph_fusion_pass_vector);

  EXPECT_EQ(SUCCESS, graph_optimizer_->OptimizeOriginalGraph(*graph));

  EXPECT_EQ(SUCCESS, graph_optimizer_->GraphFusionBeforeTransnodesInsertion(*graph));
}

TEST_F(GRAPH_FUSION_ST, converage_12) {
  EXPECT_EQ(kStagePrepare, "[GraphOpt][Prepare]");
  EXPECT_EQ(kStageBeforeQuant, "[GraphOpt][BeforeQuant]");
  EXPECT_EQ(kStageOrigin, "[GraphOpt][Origin]");
  EXPECT_EQ(kStageAftFmtSlct, "[GraphOpt][AftFmtSlct]");
  EXPECT_EQ(kStageJudgeInsert, "[GraphOpt][JdgInst]");
  EXPECT_EQ(kStageSetOpSlc, "[SubGraphOpt][SetOpSlc]");
  EXPECT_EQ(kStagePreCompile, "[SubGraphOpt][PreComp]");
  EXPECT_EQ(kStageParseCompRst, "[SubGraphOpt][ParseCompRst]");
  EXPECT_EQ(kStageLx, "[SubGraphOpt][Lx]");
  EXPECT_EQ(kStageCompile, "[SubGraphOpt][Compile]");
}
}