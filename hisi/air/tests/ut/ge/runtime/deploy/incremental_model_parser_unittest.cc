/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#define protected public
#define private public
#include "deploy/deployer/helper_deploy_planner.h"
#include "deploy/deployer/master_model_deployer.h"
#include "executor/event_handler.h"
#undef private
#undef protected

#include "graph/build/graph_builder.h"
#include "runtime/deploy/stub_models.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "generator/ge_generator.h"
#include "ge/ge_api.h"
#include "graph/ge_attr_value.h"

using namespace std;

namespace ge {
class IncrementalModelParserTest : public testing::Test {
 protected:
  void SetUp() override {
  }
  void TearDown() override {
  }

  void BuildModel(ModelBufferData &model_buffer_data) {
    //  dlog_setlevel(GE_MODULE_NAME, DLOG_DEBUG, 1);
    vector<std::string> engine_list = {"AIcoreEngine"};
    auto add1 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
    auto add2 = OP_CFG(ADD).TensorDesc(FORMAT_NCHW, DT_FLOAT, {1, 1, 224, 224});
    auto data1 = OP_CFG(DATA);
    auto data2 = OP_CFG(DATA);
    DEF_GRAPH(g1) {
      CHAIN(NODE("data_1", data1)->EDGE(0, 0)->NODE("add_1", add1));
      CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_1", add1));
      CHAIN(NODE("add_1", add1)->EDGE(0, 0)->NODE("add_2", add2));
      CHAIN(NODE("data_2", data2)->EDGE(0, 1)->NODE("add_2", add2));
    };

    auto graph = ToGeGraph(g1);
    auto compute_graph = GraphUtils::GetComputeGraph(graph);
    auto root_model = SubModels::BuildRootModel(compute_graph, false);
    auto ge_model = root_model->GetSubgraphInstanceNameToModel().at(compute_graph->GetName());
    TBEKernelStore tbe_kernel_store;
    auto kernel = std::make_shared<OpKernelBin>("hello", std::vector<char>(128));
    tbe_kernel_store.AddTBEKernel(kernel);
    tbe_kernel_store.Build();
    ge_model->SetTBEKernelStore(tbe_kernel_store);

    CustAICPUKernelStore aicpu_kernel_store;
    auto aicpu_kernel = make_shared<OpKernelBin>("world", std::vector<char>(128));
    aicpu_kernel_store.AddCustAICPUKernel(aicpu_kernel);
    aicpu_kernel_store.Build();
    ge_model->SetCustAICPUKernelStore(aicpu_kernel_store);
    EXPECT_EQ(MasterModelDeployer::SerializeModel(root_model, model_buffer_data), SUCCESS);
  }
};

TEST_F(IncrementalModelParserTest, TestInvalidBufferSize) {
  uint8_t buffer[1024];
  IncrementalModelParser parser(1024);
  EXPECT_EQ(parser.ParseAndDeserialize(0, buffer, sizeof(ModelFileHeader) - 1), PARAM_INVALID);
}

TEST_F(IncrementalModelParserTest, TestInit_InvalidHeaderLength) {
  uint8_t buffer[1024];
  auto *header = reinterpret_cast<ModelFileHeader *>(buffer);
  header->length = 1024;
  IncrementalModelParser parser(1024);
  EXPECT_EQ(parser.ParseAndDeserialize(0, buffer, sizeof(ModelFileHeader)), FAILED);
}

TEST_F(IncrementalModelParserTest, TestInit_Success) {
  uint8_t buffer[1024];
  auto *header = reinterpret_cast<ModelFileHeader *>(buffer);
  header->length = 1024 - sizeof(ModelFileHeader);
  IncrementalModelParser parser(1024);
  EXPECT_EQ(parser.Init(buffer, sizeof(buffer)), SUCCESS);
}

TEST_F(IncrementalModelParserTest, TestGetModelBeforeComplete) {
  IncrementalModelParser parser(1024);
  GeRootModelPtr root_model;
  EXPECT_EQ(parser.GetModel(root_model), FAILED);
}

TEST_F(IncrementalModelParserTest, TestInvalidPartitionNumber) {
  ModelBufferData model_buffer_data;
  BuildModel(model_buffer_data);
  auto p_tbl = reinterpret_cast<ModelPartitionTable *>(model_buffer_data.data.get() + sizeof(ModelFileHeader));
  p_tbl->num = 0;
  IncrementalModelParser parser(model_buffer_data.length);
  EXPECT_EQ(parser.ParseAndDeserialize(0, model_buffer_data.data.get(), model_buffer_data.length), PARAM_INVALID);
}

TEST_F(IncrementalModelParserTest, TestParseStaticModelSuccess) {
  ModelBufferData model_buffer_data;
  BuildModel(model_buffer_data);
  IncrementalModelParser parser(model_buffer_data.length);
  EXPECT_EQ(parser.ParseAndDeserialize(0, model_buffer_data.data.get(), model_buffer_data.length), SUCCESS);
  GeRootModelPtr root_model;
  EXPECT_EQ(parser.GetModel(root_model), SUCCESS);
}

TEST_F(IncrementalModelParserTest, TestParseAfterComplete) {
  ModelBufferData model_buffer_data;
  BuildModel(model_buffer_data);
  IncrementalModelParser parser(model_buffer_data.length);
  EXPECT_EQ(parser.ParseAndDeserialize(0, model_buffer_data.data.get(), model_buffer_data.length), SUCCESS);
  uint8_t more_data[1];
  EXPECT_EQ(parser.ParseAndDeserialize(model_buffer_data.length, more_data, 1), FAILED);
}

TEST_F(IncrementalModelParserTest, TestParseStaticModelSuccess_multi_part) {
  ModelBufferData model_buffer_data;
  BuildModel(model_buffer_data);
  IncrementalModelParser parser(model_buffer_data.length);
  EXPECT_EQ(parser.ParseAndDeserialize(0, model_buffer_data.data.get(), sizeof(ModelFileHeader)), SUCCESS);
  for (uint64_t offset = sizeof(ModelFileHeader); offset < model_buffer_data.length; ++offset) {
    auto buffer = model_buffer_data.data.get() + offset;
    EXPECT_EQ(parser.ParseAndDeserialize(offset, buffer, 1), SUCCESS);
  }
  GeRootModelPtr root_model;
  EXPECT_EQ(parser.GetModel(root_model), SUCCESS);
}

TEST_F(IncrementalModelParserTest, TestInvalidOffset) {
  ModelBufferData model_buffer_data;
  BuildModel(model_buffer_data);
  IncrementalModelParser parser(model_buffer_data.length);
  EXPECT_EQ(parser.ParseAndDeserialize(0, model_buffer_data.data.get(), sizeof(ModelFileHeader)), SUCCESS);
  EXPECT_EQ(parser.ParseAndDeserialize(sizeof(ModelFileHeader) + 1, model_buffer_data.data.get(), 1), PARAM_INVALID);
}

TEST_F(IncrementalModelParserTest, TestInvalidPartitionType) {
  ModelBufferData model_buffer_data;
  BuildModel(model_buffer_data);
  IncrementalModelParser parser(model_buffer_data.length);
  EXPECT_EQ(parser.ParseAndDeserialize(0, model_buffer_data.data.get(), sizeof(ModelFileHeader)), SUCCESS);
  EXPECT_EQ(parser.ParseAndDeserialize(sizeof(ModelFileHeader) + 1, model_buffer_data.data.get(), 1), PARAM_INVALID);
}
}  // namespace ge