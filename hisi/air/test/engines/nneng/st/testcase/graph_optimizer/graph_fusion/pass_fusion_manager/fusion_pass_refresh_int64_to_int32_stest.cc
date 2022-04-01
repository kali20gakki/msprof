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

#define protected public
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/refresh_int64_to_int32_fusion_pass.h"
#include <gtest/gtest.h>
#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#define private public
#include "common/configuration.h"
#include "common/aicore_util_constants.h"
#undef private
#undef protected


using namespace std;
using namespace ge;
using namespace fe;

namespace fe {

class STestRefreshInt64ToInt32FusionPass : public testing::Test {
 public:
  const string CONV2D = "Conv2D";
  const string NetOUTPUT = "NetOutput";

protected:
  void SetUp() {}
  void TearDown() {}

  void CreatConstDesc(const vector<int64_t> &dim_num, ge::OpDescPtr& const_desc) {
    int64_t tensor_size = 1;
    for (auto &dim : dim_num) {
      tensor_size = tensor_size * dim;
    }
    unique_ptr<int64_t[]> output_res(new (std::nothrow) int64_t[tensor_size]());
    int64_t *output_res_ptr = output_res.get();
    for(int64_t t = 0; t < tensor_size; ++t) {
      output_res_ptr[t] = int64_t(0);
    }
    // set const node
    ge::GeTensorDesc tensor_desc;
    ge::GeShape shape(dim_num);
    tensor_desc.SetShape(shape);
    tensor_desc.SetDataType(ge::DT_INT64);
    tensor_desc.SetOriginShape(shape);
    tensor_desc.SetOriginDataType(ge::DT_INT64);
    auto tensor_ptr = std::make_shared<ge::GeTensor>(tensor_desc, reinterpret_cast<uint8_t*>(output_res.get()),
      tensor_size*sizeof(int64_t));
    const_desc = ge::OpDescUtils::CreateConstOp(tensor_ptr);
  }

  void InitGraph(ComputeGraphPtr& graph) {
    vector<int64_t> dim_num = {1, 16, 14, 12};
    OpDescPtr data = std::make_shared<OpDesc>("data", fe::DATA);
    OpDescPtr weight1;
    CreatConstDesc(dim_num, weight1);
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr weight2;
    CreatConstDesc(dim_num, weight2);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr netoutput = std::make_shared<OpDesc>("netoutput", NetOUTPUT);
    // add descriptor
    ge::GeShape shape(dim_num);
    GeTensorDesc desc(shape, ge::FORMAT_NCHW, ge::DT_INT64);
    desc.SetOriginFormat(ge::FORMAT_NCHW);
    desc.SetOriginDataType(ge::DT_INT64);
    desc.SetOriginShape(shape);
    data->AddOutputDesc(desc);
    conv1->AddInputDesc(desc);
    conv1->AddInputDesc(desc);
    conv1->AddOutputDesc(desc);
    conv2->AddInputDesc(desc);
    conv2->AddInputDesc(desc);
    conv2->AddOutputDesc(desc);
    netoutput->AddInputDesc(desc);
    // create nodes
    NodePtr data_node = graph->AddNode(data);
    NodePtr weight1_node = graph->AddNode(weight1);
    NodePtr weight2_node = graph->AddNode(weight2);
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr netoutput_node = graph->AddNode(netoutput);
    ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0),
                            conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(weight1_node->GetOutDataAnchor(0),
                            conv1_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            conv2_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(weight2_node->GetOutDataAnchor(0),
                            conv2_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            netoutput_node->GetInDataAnchor(0));
  }
};

TEST_F(STestRefreshInt64ToInt32FusionPass, success_case1) {
  string old_version = Configuration::Instance(fe::AI_CORE_NAME).soc_version_ ;
  Configuration::Instance(fe::AI_CORE_NAME).soc_version_ = SOC_VERSION_HI3796CV300CS;
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  InitGraph(graph);
  RefreshInt64ToInt32FusionPass pass;
  vector<GraphPass*> passes = {&pass};
  Status status = PassManager::Run(*graph, passes);
  for (auto &node : graph->GetDirectNode()) {
    auto op_desc = node->GetOpDesc();
    for (auto &input_desc : op_desc->GetAllInputsDesc()) {
      EXPECT_NE(input_desc.GetDataType(), ge::DT_INT64);
    }
    for (auto &output_desc : op_desc->GetAllOutputsDesc()) {
      EXPECT_NE(output_desc.GetDataType(), ge::DT_INT64);
    }
  }
  bool need_refresh_int64 = true;
  string refresh_flag = "refresh_int64_flag";
  ge::AttrUtils::GetBool(graph, refresh_flag, need_refresh_int64);
  EXPECT_EQ(need_refresh_int64, false);

  status = PassManager::Run(*graph, passes);
  ge::AttrUtils::GetBool(graph, refresh_flag, need_refresh_int64);
  EXPECT_EQ(need_refresh_int64, false);
  Configuration::Instance(fe::AI_CORE_NAME).soc_version_ = old_version;
}
}  // namespace fe
