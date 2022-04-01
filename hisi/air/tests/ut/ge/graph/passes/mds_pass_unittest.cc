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

#include "graph/attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "../passes/graph_builder_utils.h"
#include "ge_graph_dsl/graph_dsl.h"
#define protected public
#define private public
#include "inc/graph/ge_local_context.h"
#include "graph/passes/mds_pass.h"
#undef protected
#undef private

using namespace std;
using namespace testing;
using namespace ge;
USING_GE_NS
namespace {
ge::ComputeGraphPtr BuildGraph(const vector<int64_t> &data_dims) {
  ge::ut::GraphBuilder builder("graph");
  auto data = builder.AddNode("data", "Data", 1, 1, FORMAT_NCHW, DT_FLOAT, data_dims);
  auto var = builder.AddNode("var", "Variable", 1, 1, FORMAT_NCHW, DT_FLOAT, data_dims);
  auto cut_node = builder.AddNode("cut_enable_node", "Cut", 2, 1, FORMAT_NCHW, DT_FLOAT, data_dims);
  auto no_cut_node = builder.AddNode("no_cut_node", "NoCut", 1, 1, FORMAT_NCHW, DT_FLOAT, data_dims);
  auto gradient_compute_node = builder.AddNode("gradient_compute_node", "Grad", 1, 1, FORMAT_NCHW, DT_FLOAT, data_dims);
  auto out_node = builder.AddNode("out_node", "Out", 1, 1, FORMAT_NCHW, DT_FLOAT, data_dims);
  (void)AttrUtils::SetListListInt(var->GetOpDesc()->MutableOutputDesc(0), ATTR_NAME_CUT_INFO, {{1, 0, 0, 0}});
  (void)AttrUtils::SetListListInt(cut_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_CUT_INFO, {{1, 0, 0, 0}});
  (void)AttrUtils::SetListListInt(cut_node->GetOpDesc()->MutableInputDesc(1), ATTR_NAME_CUT_INFO, {{1, 0, 0, 0}});
  (void)AttrUtils::SetListListInt(cut_node->GetOpDesc()->MutableOutputDesc(0), ATTR_NAME_CUT_INFO, {{1, 0, 0, 0}});
  (void)AttrUtils::SetListListInt(no_cut_node->GetOpDesc()->MutableInputDesc(0), ATTR_NAME_CUT_INFO, {{0, 0, 0, 0}});
  (void)AttrUtils::SetListListInt(no_cut_node->GetOpDesc()->MutableOutputDesc(0), ATTR_NAME_CUT_INFO, {{0, 0, 0, 0}});
  (void)AttrUtils::SetBool(gradient_compute_node->GetOpDesc(), ATTR_NAME_GRADIENT_NODE, true);

  builder.AddDataEdge(data, 0, cut_node, 0);
  builder.AddDataEdge(var, 0, cut_node, 1);
  builder.AddDataEdge(cut_node, 0, no_cut_node, 0);
  builder.AddDataEdge(no_cut_node, 0, gradient_compute_node, 0);
  builder.AddDataEdge(gradient_compute_node, 0, out_node, 0);
  for (const auto &node : builder.GetGraph()->GetDirectNode()) {
    AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_ENGINE_NAME_FOR_LX, "AIcoreEngine");
    AttrUtils::SetStr(node->GetOpDesc(), ATTR_NAME_KKERNEL_LIB_NAME_FOR_LX, "AiCoreLib");
  }

  return builder.GetGraph();
}
}  // namespace

class MdsUtilsTest : public testing::Test {
 public:
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(MdsUtilsTest, GatherSuccessNotChange) {
  auto graph = BuildGraph({2, 2, 224, 224});
  auto before_size = graph->GetDirectNodesSize();

  auto cut_node = graph->FindNode("cut_enable_node");
  EXPECT_NE(cut_node, nullptr);
  auto no_cut_node = graph->FindNode("no_cut_node");
  EXPECT_NE(no_cut_node, nullptr);
  auto ret = MdsUtils::DataGather(cut_node->GetOutDataAnchor(0), no_cut_node->GetInDataAnchor(0));
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), before_size);
}
TEST_F(MdsUtilsTest, GatherSuccessChanged) {
  auto graph = BuildGraph({2, 2, 224, 224});
  auto before_size = graph->GetDirectNodesSize();

  auto cut_node = graph->FindNode("cut_enable_node");
  EXPECT_NE(cut_node, nullptr);
  auto no_cut_node = graph->FindNode("no_cut_node");
  EXPECT_NE(no_cut_node, nullptr);
  cut_node->GetOpDesc()->MutableOutputDesc(0)->SetShape(GeShape({1, 2, 224, 224}));
  auto ret = MdsUtils::DataGather(cut_node->GetOutDataAnchor(0), no_cut_node->GetInDataAnchor(0));
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_NE(graph->GetDirectNodesSize(), before_size);
}

TEST_F(MdsUtilsTest, SliceSuccessSliced) {
  auto graph = BuildGraph({2, 2, 224, 224});
  auto before_size = graph->GetDirectNodesSize();
  auto data_node = graph->FindNode("data");
  EXPECT_NE(data_node, nullptr);
  auto cut_node = graph->FindNode("cut_enable_node");
  EXPECT_NE(cut_node, nullptr);

  NodePtr input_node{nullptr};
  auto ret = MdsUtils::DataSlice(data_node->GetOutDataAnchor(0), cut_node->GetInDataAnchor(0), CutType::kCutN,
                                 input_node);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_NE(graph->GetDirectNodesSize(), before_size);
}

TEST_F(MdsUtilsTest, IsDistributedDeploySupportedFalse) {
  GeTensorDescPtr ge_tensor_desc_ptr;
  EXPECT_EQ(MdsUtils::IsDistributedDeploySupported(ge_tensor_desc_ptr, CutType::kCutN), false);
  ge_tensor_desc_ptr = MakeShared<GeTensorDesc>(GeShape({2}), FORMAT_ND, DT_FLOAT);
  EXPECT_EQ(MdsUtils::IsDistributedDeploySupported(ge_tensor_desc_ptr, CutType::kCutN), false);
  EXPECT_EQ(MdsUtils::IsDistributedDeploySupported(ge_tensor_desc_ptr, CutType::kDynamicCutAll), false);
  ge_tensor_desc_ptr->SetFormat(FORMAT_NCHW);
  EXPECT_EQ(MdsUtils::IsDistributedDeploySupported(ge_tensor_desc_ptr, CutType::kCutN), false);
  ge_tensor_desc_ptr->SetShape(GeShape({2, 2, 2, 2}));
  AttrUtils::SetListListInt(ge_tensor_desc_ptr, ATTR_NAME_CUT_INFO, {});
  EXPECT_EQ(MdsUtils::IsDistributedDeploySupported(ge_tensor_desc_ptr, CutType::kCutN), false);
}

TEST_F(MdsUtilsTest, SliceSuccessNotChanged) {
  auto graph = BuildGraph({2, 2, 224, 224});
  auto before_size = graph->GetDirectNodesSize();
  auto data_node = graph->FindNode("data");
  EXPECT_NE(data_node, nullptr);
  auto cut_node = graph->FindNode("cut_enable_node");
  EXPECT_NE(cut_node, nullptr);
  cut_node->GetOpDesc()->MutableInputDesc(0)->SetShape(GeShape({1, 2, 224, 224}));
  NodePtr input_node{nullptr};
  auto ret = MdsUtils::DataSlice(data_node->GetOutDataAnchor(0), cut_node->GetInDataAnchor(0), CutType::kCutN,
                                 input_node);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(graph->GetDirectNodesSize(), before_size);
}

TEST_F(MdsUtilsTest, ReduceSuccess) {
  auto graph = BuildGraph({2, 2, 224, 224});
  auto before_size = graph->GetDirectNodesSize();
  auto gradient_compute_node = graph->FindNode("gradient_compute_node");
  EXPECT_NE(gradient_compute_node, nullptr);
  auto out_node = graph->FindNode("out_node");
  EXPECT_NE(out_node, nullptr);

  auto ret = MdsUtils::DataReduce(gradient_compute_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_NE(graph->GetDirectNodesSize(), before_size);
}

TEST_F(MdsUtilsTest, GetNLocation) {
  EXPECT_EQ(MdsUtils::GetNLocation(FORMAT_CHWN), static_cast<int64_t>(HCutIndex::kHLocation3));
  EXPECT_EQ(MdsUtils::GetNLocation(FORMAT_NHWC), static_cast<int64_t>(HCutIndex::kHLocation0));
  EXPECT_EQ(MdsUtils::GetNLocation(FORMAT_HWCN), static_cast<int64_t>(HCutIndex::kHLocation3));
  EXPECT_EQ(MdsUtils::GetNLocation(FORMAT_FRACTAL_Z_C04), static_cast<int64_t>(HCutIndex::kHInvalidLocation));
}

TEST_F(MdsUtilsTest, GetHLocation) {
  EXPECT_EQ(MdsUtils::GetHLocation(FORMAT_CHWN), static_cast<int64_t>(HCutIndex::kHLocation1));
  EXPECT_EQ(MdsUtils::GetHLocation(FORMAT_NHWC), static_cast<int64_t>(HCutIndex::kHLocation1));
  EXPECT_EQ(MdsUtils::GetHLocation(FORMAT_HWCN), static_cast<int64_t>(HCutIndex::kHLocation0));
  EXPECT_EQ(MdsUtils::GetHLocation(FORMAT_FRACTAL_Z_C04), static_cast<int64_t>(HCutIndex::kHInvalidLocation));
}

TEST_F(MdsUtilsTest, IsMDSNeeded) {
  EXPECT_EQ(false, MdsUtils::IsMDSNeeded());

  std::map<std::string, std::string> options;
  options.emplace(OPTION_DEVICE_TYPE, kDeviceChipType);
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_EQ(true, MdsUtils::IsMDSNeeded());
  options[OPTION_DEVICE_TYPE] = "invalid_type";
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_EQ(false, MdsUtils::IsMDSNeeded());
}

TEST_F(MdsUtilsTest, SetDeviceV2) {
  std::map<std::string, std::string> options;
  options.emplace(OPTION_DEVICE_TYPE, kDeviceChipType);
  ge::GetThreadLocalContext().SetGraphOption(options);
  EXPECT_EQ(MdsUtils::SetDevice(0), SUCCESS);
}
class ModelDeploySchedulerPassTest : public testing::Test {
 public:
 protected:
  void SetUp() {}
  void TearDown() {}
};
TEST_F(ModelDeploySchedulerPassTest, NoCutNoDeploy) {
  auto graph = BuildGraph({1, 1, 1, 224});  // NCHW as default
  auto before_size = graph->GetDirectNodesSize();

  ModelDeploySchedulerPass model_deploy_scheduler_pass;
  std::map<std::string, std::string> options;
  options.emplace(OPTION_DEVICE_TYPE, kDeviceDieType);
  ge::GetThreadLocalContext().SetGraphOption(options);
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(model_deploy_scheduler_pass.Run(graph), SUCCESS);
  auto after_size = graph->GetDirectNodesSize();
  EXPECT_EQ(after_size, before_size);

  vector<GeAttrValue::NAMED_ATTRS> deployInfo;
  EXPECT_EQ(ge::AttrUtils::GetListNamedAttrs(graph, ATTR_NAME_DEPLOY_INFO, deployInfo), false);
  EXPECT_EQ(deployInfo.size(), 0);
}

TEST_F(ModelDeploySchedulerPassTest, NoCutButDeploy) {
  auto graph = BuildGraph({1, 1, 1, 224});  // NCHW as default
  auto before_size = graph->GetDirectNodesSize();

  ModelDeploySchedulerPass model_deploy_scheduler_pass;
  std::map<std::string, std::string> options;
  options.emplace(OPTION_DEVICE_TYPE, kDeviceChipType);
  ge::GetThreadLocalContext().SetGraphOption(options);
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(model_deploy_scheduler_pass.Run(graph), SUCCESS);
  auto after_size = graph->GetDirectNodesSize();
  EXPECT_EQ(after_size, before_size);

  vector<GeAttrValue::NAMED_ATTRS> deployInfo;
  EXPECT_EQ(ge::AttrUtils::GetListNamedAttrs(graph, ATTR_NAME_DEPLOY_INFO, deployInfo), true);
  EXPECT_EQ(deployInfo.size(), 2);
}

TEST_F(ModelDeploySchedulerPassTest, CutN) {
  auto graph = BuildGraph({2, 1, 224, 224});  // NCHW as default
  auto before_size = graph->GetDirectNodesSize();
  ModelDeploySchedulerPass model_deploy_scheduler_pass;
  std::map<std::string, std::string> options;
  options.emplace(OPTION_DEVICE_TYPE, kDeviceChipType);
  ge::GetThreadLocalContext().SetGraphOption(options);
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(model_deploy_scheduler_pass.Run(graph), SUCCESS);
  auto after_size = graph->GetDirectNodesSize();
  EXPECT_NE(after_size, before_size);

  vector<GeAttrValue::NAMED_ATTRS> deployInfo;
  EXPECT_EQ(ge::AttrUtils::GetListNamedAttrs(graph, ATTR_NAME_DEPLOY_INFO, deployInfo), true);
  EXPECT_EQ(deployInfo.size(), kDeployNumber);
}
TEST_F(ModelDeploySchedulerPassTest, CutHWithOutNodeSupport) {
  auto graph = BuildGraph({1, 1, 224, 224});  // NCHW as default
  auto before_size = graph->GetDirectNodesSize();
  ModelDeploySchedulerPass model_deploy_scheduler_pass;
  std::map<std::string, std::string> options;
  options.emplace(OPTION_DEVICE_TYPE, kDeviceChipType);
  ge::GetThreadLocalContext().SetGraphOption(options);
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);
  EXPECT_EQ(model_deploy_scheduler_pass.Run(graph), SUCCESS);
  vector<GeAttrValue::NAMED_ATTRS> deployInfo;
  EXPECT_EQ(ge::AttrUtils::GetListNamedAttrs(graph, ATTR_NAME_DEPLOY_INFO, deployInfo), true);
  EXPECT_EQ(deployInfo.size(), kDeployNumber);
}
