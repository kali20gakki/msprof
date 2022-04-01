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
#include <stdio.h>
#include <map>
#include <memory>

#include "proto/om.pb.h"

#define protected public
#define private public
#include "common/graph_comm.h"
#include "common/pass_manager.h"
#include "common/configuration.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/concat_quant_fusion_pass.h"
#include "graph_optimizer/ub_fusion/automatic_buffer_fusion.h"
#include "graph_optimizer/graph_fusion/graph_fusion.h"

#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

class STEST_st_check_graph_cycle : public testing::Test {
 protected:
  virtual void SetUp() {

  }

  virtual void TearDown() {

  }
  void SetPattern(ge::OpDescPtr opdef, string optype) {
    auto key_pattern = opdef->GetName() + "_pattern";
    ge::AttrUtils::SetStr(opdef, key_pattern, optype);
  }
  void SetTvmType(ge::OpDescPtr opdef) {
    ge::AttrUtils::SetInt(opdef, ge::ATTR_NAME_IMPLY_TYPE, static_cast<int64_t>(domi::ImplyType::TVM));
  }

  void BuildGraph(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr addn = std::make_shared<OpDesc>("addn", "AddN");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1","ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2","ReLU");
    SetPattern(addn, "ElemWise");
    SetPattern(elemwise, "ElemWise");

    SetTvmType(addn);
    SetTvmType(elemwise);
    SetTvmType(relu);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    data1->AddOutputDesc(tenosr_desc);
    data2->AddOutputDesc(tenosr_desc);
    addn->AddInputDesc(tenosr_desc);
    addn->AddInputDesc(tenosr_desc);
    addn->AddOutputDesc(tenosr_desc);
    elemwise->AddInputDesc(tenosr_desc);
    elemwise->AddOutputDesc(tenosr_desc);
    relu->AddInputDesc(tenosr_desc);
    relu->AddInputDesc(tenosr_desc);
    relu->AddOutputDesc(tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);
    AttrUtils::SetInt(addn, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr addn_node = graph->AddNode(addn);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(addn_node->GetName(), std::move(buffer));
    addn_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(addn_node->GetOutDataAnchor(0), elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu2_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(1));
  }

  void BuildGraph2(ComputeGraphPtr graph) {
    OpDescPtr data = std::make_shared<OpDesc>("DATA0", fe::DATA);
    OpDescPtr data1 = std::make_shared<OpDesc>("DATA1", fe::DATA);
    OpDescPtr data2 = std::make_shared<OpDesc>("DATA2", fe::DATA);
    OpDescPtr addn = std::make_shared<OpDesc>("addn", "AddN");
    OpDescPtr elemwise = std::make_shared<OpDesc>("elem", "Eltwise");
    OpDescPtr relu = std::make_shared<OpDesc>("relu", "ReLU");
    OpDescPtr relu1 = std::make_shared<OpDesc>("relu1","ReLU");
    OpDescPtr relu2 = std::make_shared<OpDesc>("relu2","ReLU");
    SetPattern(addn, "ElemWise");
    SetPattern(elemwise, "ElemWise");

    SetTvmType(addn);
    SetTvmType(elemwise);
    SetTvmType(relu);
    // add descriptor
    vector<int64_t> dim(4, 4);
    GeShape shape(dim);
    GeTensorDesc tenosr_desc(shape);

    data->AddOutputDesc(tenosr_desc);
    data1->AddOutputDesc(tenosr_desc);
    data2->AddOutputDesc(tenosr_desc);
    addn->AddInputDesc(tenosr_desc);
    addn->AddInputDesc(tenosr_desc);
    addn->AddOutputDesc(tenosr_desc);
    elemwise->AddInputDesc(tenosr_desc);
    elemwise->AddOutputDesc(tenosr_desc);
    relu->AddInputDesc(tenosr_desc);
    relu->AddOutputDesc(tenosr_desc);
    relu1->AddInputDesc(tenosr_desc);
    relu1->AddOutputDesc(tenosr_desc);
    relu2->AddInputDesc(tenosr_desc);
    relu2->AddOutputDesc(tenosr_desc);
    AttrUtils::SetInt(addn, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(elemwise, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(relu, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);

    NodePtr data_node = graph->AddNode(data);
    NodePtr data1_node = graph->AddNode(data1);
    NodePtr data2_node = graph->AddNode(data2);
    NodePtr addn_node = graph->AddNode(addn);
    NodePtr elemwise_node = graph->AddNode(elemwise);
    NodePtr relu_node = graph->AddNode(relu);
    NodePtr relu1_node = graph->AddNode(relu1);
    NodePtr relu2_node = graph->AddNode(relu2);
    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin+strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(addn_node->GetName(), std::move(buffer));
    addn_node->GetOpDesc()->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), addn_node->GetInDataAnchor(1));
    GraphUtils::AddEdge(addn_node->GetOutDataAnchor(0), elemwise_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(elemwise_node->GetOutDataAnchor(0), relu1_node->GetInDataAnchor(0));
    GraphUtils::AddEdge(relu1_node->GetOutDataAnchor(0), relu2_node->GetInDataAnchor(0));
  }
};

TEST_F(STEST_st_check_graph_cycle, check_graph_cycle_yes) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph(graph_out);
  GraphFusion graphFusion(nullptr, nullptr, nullptr, nullptr);
  setenv("ENABLE_NETWORK_ANALYSIS_DEBUG", "1", true);
  Configuration config(fe::AI_CORE_NAME);
  std::map<string, string> options;
  string soc_version = "Ascend310";
  config.Initialize(options, soc_version);
  bool ret = false;
  if (config.IsEnableNetworkAnalysis()) {
    ret = graphFusion.CheckGraphCycle(*graph_out);
  }

  EXPECT_EQ(ret, true);
}

TEST_F(STEST_st_check_graph_cycle, check_graph_cycle_no) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph2(graph_out);
  GraphFusion graphFusion(nullptr, nullptr, nullptr, nullptr);
  setenv("ENABLE_NETWORK_ANALYSIS_DEBUG", "1", true);
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend310";
  config.Initialize(options, soc_version);
  bool ret = true;
  if (config.IsEnableNetworkAnalysis()) {
    ret = graphFusion.CheckGraphCycle(*graph_out);
  }

  EXPECT_EQ(ret, false);
}

TEST_F(STEST_st_check_graph_cycle, check_graph_cycle_pass_success) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph(graph_out);
  GraphFusion graphFusion(nullptr, nullptr, nullptr, nullptr);
  setenv("ENABLE_NETWORK_ANALYSIS_DEBUG", "1", true);
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend310";
  config.Initialize(options, soc_version);
  fe::FusionPassOrRule pass_or_rule("PaddDepthwiseConv2dFusionPass", 0, PASS_METHOD, BUILT_IN_PASS_PRIORITY_MIN,
                                []() -> ::fe::GraphPass * { return new (std::nothrow) ConcatQuantFusionPass(); });
  Status status = graphFusion.RunOnePassFusion(*graph_out, pass_or_rule);

  EXPECT_EQ(status, fe::SUCCESS);
}

TEST_F(STEST_st_check_graph_cycle, report_cycle_after_pass_fusion) {
  ComputeGraphPtr graph_out = std::make_shared<ComputeGraph>("test");
  BuildGraph(graph_out);
  GraphFusion graphFusion(nullptr, nullptr, nullptr, nullptr);
  setenv("ENABLE_NETWORK_ANALYSIS_DEBUG", "1", true);
  Configuration config(fe::AI_CORE_NAME);
  map<string, string> options;
  string soc_version = "Ascend310";
  config.Initialize(options, soc_version);
  FusionPassOrRule pass_or_rule("ConcatQuantFusionPass",
                               0, PASS_METHOD,
                               BUILT_IN_PASS_PRIORITY_MIN,
                               []() -> ::fe::GraphPass * { return new (std::nothrow) ConcatQuantFusionPass();});  
  graphFusion.ReportAfterCheckGraphCycle(*graph_out, pass_or_rule, GRAPH_FUSION_PASS_TYPE_RESERVED);
}
