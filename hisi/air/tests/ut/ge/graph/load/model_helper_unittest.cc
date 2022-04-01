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

#include <stdio.h>
#include <gtest/gtest.h>
#define private public
#define protected public
#include "framework/common/helper/model_helper.h"
#include "framework/omg/ge_init.h"
#include "common/model/ge_model.h"
#include "common/model_parser/model_parser.h"
#include "graph/buffer_impl.h"
#include "graph/utils/graph_utils.h"
#include "graph/debug/ge_attr_define.h"
#undef private
#undef protected

#include "proto/task.pb.h"

using namespace std;

namespace ge {
class UtestModelHelper : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtestModelHelper, save_size_to_modeldef)
{
  GeModelPtr ge_model = ge::MakeShared<ge::GeModel>();
  std::shared_ptr<domi::ModelTaskDef> task = ge::MakeShared<domi::ModelTaskDef>();
  ge_model->SetModelTaskDef(task);
  ModelHelper model_helper;
  EXPECT_EQ(SUCCESS, model_helper.SaveSizeToModelDef(ge_model));
}

TEST_F(UtestModelHelper, SaveModelPartitionInvalid)
{
  std::shared_ptr<OmFileSaveHelper> om_file_save_helper = std::make_shared<OmFileSaveHelper>();
  ModelPartitionType type = MODEL_DEF;
  uint8_t data[128] = {0};
  size_t size = 10000000000;
  size_t model_index = 0;
  ModelHelper model_helper;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), PARAM_INVALID);
  type = WEIGHTS_DATA;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), PARAM_INVALID);
  type = TASK_INFO;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), PARAM_INVALID);
  type = TBE_KERNELS;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), PARAM_INVALID);
  type = CUST_AICPU_KERNELS;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), PARAM_INVALID);
  type = (ModelPartitionType)100;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), PARAM_INVALID);
  size = 1024;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, nullptr, size, model_index), PARAM_INVALID);
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), SUCCESS);
  om_file_save_helper->context_.model_data_len_ = 4000000000;
  model_index = 2000000000;
  EXPECT_EQ(model_helper.SaveModelPartition(om_file_save_helper, type, (uint8_t*)data, size, model_index), PARAM_INVALID);
}

TEST_F(UtestModelHelper, SaveOriginalGraphToOmModel)
{
  Graph graph("graph");
  std::string output_file = "";
  ModelHelper model_helper;
  EXPECT_EQ(model_helper.SaveOriginalGraphToOmModel(graph, output_file), FAILED);
  output_file = "output.graph";
  EXPECT_EQ(model_helper.SaveOriginalGraphToOmModel(graph, output_file), FAILED);
  ge::OpDescPtr add_op(new ge::OpDesc("add1", "Add"));
  add_op->AddDynamicInputDesc("input", 2);
  add_op->AddDynamicOutputDesc("output", 1);
  std::shared_ptr<ge::ComputeGraph> compute_graph(new ge::ComputeGraph("test_graph"));
  auto add_node = compute_graph->AddNode(add_op);
  auto graph2 = ge::GraphUtils::CreateGraphFromComputeGraph(compute_graph);
  EXPECT_EQ(model_helper.SaveOriginalGraphToOmModel(graph2, output_file), SUCCESS);
}

TEST_F(UtestModelHelper, GetGeModel)
{
  ModelHelper model_helper;
  model_helper.model_ = nullptr;
  EXPECT_NE(model_helper.GetGeModel(), nullptr);
}

TEST_F(UtestModelHelper, LoadTask)
{
  ModelHelper model_helper;
  OmFileLoadHelper om_load_helper;
  GeModelPtr cur_model = std::make_shared<GeModel>();
  size_t mode_index = 10;
  EXPECT_EQ(model_helper.LoadTask(om_load_helper, cur_model, mode_index), FAILED);
}

TEST_F(UtestModelHelper, LoadTaskByHelper)
{
  ModelHelper model_helper;
  OmFileLoadHelper om_load_helper;
  om_load_helper.is_inited_ = false;
  EXPECT_EQ(model_helper.LoadTask(om_load_helper), FAILED);
  om_load_helper.is_inited_ = true;
  ModelPartition mp;
  mp.type = TASK_INFO;
  om_load_helper.context_.partition_datas_.push_back(mp);
  model_helper.model_ = std::make_shared<GeModel>();
  EXPECT_EQ(model_helper.LoadTask(om_load_helper), SUCCESS);
}

TEST_F(UtestModelHelper, SaveToOmRootModel)
{
  ModelHelper model_helper;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  GeRootModelPtr ge_root_model = std::make_shared<GeRootModel>(graph);
  ge_root_model->subgraph_instance_name_to_model_["graph"] = std::make_shared<GeModel>();
  SaveParam save_param;
  std::string output_file = "output.file";
  ModelBufferData model;
  bool is_unknown_shape = true;
  EXPECT_EQ(model_helper.SaveToOmRootModel(ge_root_model, save_param, output_file, model, is_unknown_shape), INTERNAL_ERROR);
  ge_root_model->subgraph_instance_name_to_model_["graph2"] = std::make_shared<GeModel>();
  EXPECT_EQ(model_helper.SaveToOmRootModel(ge_root_model, save_param, output_file, model, is_unknown_shape), INTERNAL_ERROR);
}

TEST_F(UtestModelHelper, SaveModelDef)
{
  ModelHelper model_helper;
  std::shared_ptr<OmFileSaveHelper> om_file_save_helper = std::make_shared<OmFileSaveHelper>();
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge::Buffer model_buffer;
  model_buffer.impl_->buffer_ = nullptr;
  size_t model_index = 0;
  EXPECT_EQ(model_helper.SaveModelDef(om_file_save_helper, ge_model, model_buffer, model_index), SUCCESS);
}

TEST_F(UtestModelHelper, SaveAllModelPartiton)
{
  ModelHelper model_helper;
  std::shared_ptr<OmFileSaveHelper> om_file_save_helper = std::make_shared<OmFileSaveHelper>();
  GeModelPtr ge_model = std::make_shared<GeModel>();
  ge::Buffer model_buffer;
  model_buffer.impl_->buffer_ = nullptr;
  ge::Buffer task_buffer;
  size_t model_index = 0;
  EXPECT_EQ(model_helper.SaveAllModelPartiton(om_file_save_helper, ge_model, model_buffer, task_buffer, model_index), FAILED);
}

TEST_F(UtestModelHelper, SaveToOmModel)
{
  ModelHelper model_helper;
  GeModelPtr ge_model = std::make_shared<GeModel>();
  SaveParam save_param;
  std::string output_file = "";
  ModelBufferData model;
  EXPECT_EQ(model_helper.SaveToOmModel(ge_model, save_param, output_file, model), FAILED);
}

TEST_F(UtestModelHelper, LoadFromFile_failed) {
  ModelParserBase base;
  ge::ModelData model_data;
  EXPECT_EQ(base.LoadFromFile("/tmp/123test", -1, model_data), ACL_ERROR_GE_EXEC_MODEL_PATH_INVALID);
}

TEST_F(UtestModelHelper, GetModelNameFromMergedGraphName)
{
  ModelHelper model_helper;
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("graph");
  std::string model_name;
  EXPECT_EQ(model_helper.GetModelNameFromMergedGraphName(graph, model_name), SUCCESS);
  EXPECT_EQ(model_name, "graph");

  graph->SetName("graph_1");
  EXPECT_EQ(model_helper.GetModelNameFromMergedGraphName(graph, model_name), SUCCESS);
  EXPECT_EQ(model_name, "graph");

  std::string process_node_engine_id = "NPU";
  graph->SetName("graph_" + process_node_engine_id);
  (void)ge::AttrUtils::SetStr(graph, ge::ATTR_NAME_PROCESS_NODE_ENGINE_ID, process_node_engine_id);
  EXPECT_EQ(model_helper.GetModelNameFromMergedGraphName(graph, model_name), SUCCESS);
  EXPECT_EQ(model_name, "graph");

  graph->SetName("graph_" + process_node_engine_id + "_1");
  EXPECT_EQ(model_helper.GetModelNameFromMergedGraphName(graph, model_name), SUCCESS);
  EXPECT_EQ(model_name, "graph");

  graph->SetName("graph_NPU_" + process_node_engine_id + "_1");
  EXPECT_EQ(model_helper.GetModelNameFromMergedGraphName(graph, model_name), SUCCESS);
  EXPECT_EQ(model_name, "graph_NPU");
}
}  // namespace ge
