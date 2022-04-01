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
#include "external/ge/ge_api.h"
#include "ge_running_env/ge_running_env_faker.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "ge_graph_dsl/assert/graph_assert.h"
#include "graph/compute_graph.h"
#include "graph/op_desc.h"
#include "framework/common/types.h"
#include "graph/ge_local_context.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "graph/execute/model_executor.h"
#include "graph/load/model_manager/model_manager.h"
#include "graph/load/graph_loader.h"
#include "graph/passes/mds_pass.h"

namespace ge {
class TestModelDeploySchedule : public testing::Test {
 protected:
  void SetUp() { ge_env.InstallDefault(); }
  void TearDown() {}
  GeRunningEnvFaker ge_env;
};
TEST_F(TestModelDeploySchedule, test_model_compile_cutn) {
  DEF_GRAPH(g1) {
    auto conv = std::make_shared<OpDesc>("conv1", CONV2D);
    GeTensorDesc tensor_desc(ge::GeShape({2, 1, 224, 224}), FORMAT_NCHW, DT_INT64);
    // conv input 0
    conv->AddInputDesc(tensor_desc);
    ge::AttrUtils::SetListListInt(conv->MutableInputDesc(0), "_cut_info", {{1, 0, 0, 0}});
    ge::AttrUtils::SetInt(conv->MutableInputDesc(0), ATTR_NAME_SPECIAL_INPUT_SIZE, 224*224*2*8*3/2);
    // conv input 1
    conv->AddInputDesc(tensor_desc);
    ge::AttrUtils::SetListListInt(conv->MutableInputDesc(1), "_cut_info", {{0, 0, 0, 0}});
    conv->AddOutputDesc(tensor_desc);
    // conv output
    ge::AttrUtils::SetListListInt(conv->MutableOutputDesc(0), "_cut_info", {{1, 0, 0, 0}});
    ge::AttrUtils::SetInt(conv->MutableOutputDesc(0), ATTR_NAME_SPECIAL_OUTPUT_SIZE, 224*224*2*8*3/2);
    ge::AttrUtils::SetStr(conv, "_alias_engine_name", "AiCoreLib");
    auto data = std::make_shared<OpDesc>("data1", DATA);
    (void) data->AddOutputDesc(tensor_desc);
    (void) data->AddInputDesc(tensor_desc);

    CHAIN(NODE(data)->NODE(conv));
    CHAIN(NODE("var1", VARIABLE)->NODE(conv));
    CHAIN(NODE(conv)->NODE("relu", RELU)->NODE("netoutput", NETOUTPUT));
    CHAIN(NODE("relu")
              ->NODE("gradients/relu_grad", RELU6GRAD)
              ->NODE("myallrecue", HCOMALLREDUCE)
              ->EDGE(0, 1)
              ->NODE("netoutput"));
  };

  map<AscendString, AscendString> options;
  options.emplace(ge::OPTION_DEVICE_TYPE, kDeviceChipType);
  Session session(options);
  session.AddGraph(1, ToGeGraph(g1), options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_1");
    ASSERT_NE(graph->GetAllNodesSize(), 4);
  };
}

TEST_F(TestModelDeploySchedule, test_model_compile_cuth) {
  DEF_GRAPH(g1) {
    auto conv = std::make_shared<OpDesc>("conv1", CONV2D);
    GeTensorDesc tensor_desc(ge::GeShape({1, 1, 224, 224}), FORMAT_NCHW, DT_INT64);
    // conv input 0
    conv->AddInputDesc(tensor_desc);
    ge::AttrUtils::SetListListInt(conv->MutableInputDesc(0), "_cut_info", {{1, 0, 0, 0}});
    // conv input 1
    conv->AddInputDesc(tensor_desc);
    ge::AttrUtils::SetListListInt(conv->MutableInputDesc(1), "_cut_info", {{0, 0, 0, 0}});
    conv->AddOutputDesc(tensor_desc);
    ge::AttrUtils::SetListListInt(conv->MutableOutputDesc(0), "_cut_info", {{1, 0, 0, 0}});
    auto data = std::make_shared<OpDesc>("data1", DATA);
    (void) data->AddOutputDesc(tensor_desc);
    (void) data->AddInputDesc(tensor_desc);

    CHAIN(NODE(data)->NODE(conv));
    CHAIN(NODE("var1", VARIABLE)->NODE(conv));
    CHAIN(NODE(conv)->NODE("relu", RELU)->NODE("netoutput", NETOUTPUT));
    CHAIN(NODE("relu")
              ->NODE("gradients/relu_grad", RELU6GRAD)
              ->EDGE(0, 1)
              ->NODE("netoutput"));
  };

  map<AscendString, AscendString> options;
  options.emplace(ge::OPTION_DEVICE_TYPE, kDeviceChipType);
  Session session(options);
  session.AddGraph(1, ToGeGraph(g1), options);
  std::vector<InputTensorInfo> inputs;
  auto ret = session.BuildGraph(1, inputs);
  EXPECT_EQ(ret, SUCCESS);

  CHECK_GRAPH(PreRunAfterBuild) {
    std::string pne_name;
    (void)AttrUtils::GetStr(graph, ATTR_NAME_PROCESS_NODE_ENGINE_ID, pne_name);
    EXPECT_EQ(graph->GetName(), "g1_" + pne_name + "_1");
    ASSERT_NE(graph->GetAllNodesSize(), 4);
  };
}

}