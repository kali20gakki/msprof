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

#define protected public
#define private public
#include "exec_runtime/deploy/deploy_planner.h"
#undef private
#undef protected

#include "graph/passes/graph_builder_utils.h"
#include "graph/build/graph_builder.h"
#include "runtime/deploy/stub_models.h"

using namespace std;

namespace ge {
class DeployPlannerTest : public testing::Test {
 protected:
  void SetUp() override {
  }
  void TearDown() override {
  }
};

TEST_F(DeployPlannerTest, TestFailedDueToMismatchOfQueueNames) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithQueueBindings());
  ASSERT_TRUE(root_model != nullptr);
  root_model->model_relation_->submodel_queue_infos["subgraph-2"].input_queue_names[0] = "oops";
  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  auto ret = DeployPlanner(flow_model).BuildPlan(deploy_plan);
  ASSERT_EQ(ret, PARAM_INVALID);
}

///      NetOutput
///         |
///         |
///        PC_2
///        |  \.
///       PC_1 |
///     /     \.
///    |      |
///  data1  data2
TEST_F(DeployPlannerTest, TestBuildDeployPlan_WithQueueBindings) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithQueueBindings());
  ASSERT_TRUE(root_model != nullptr);
  EXPECT_EQ(root_model->GetSubmodels().size(), 2);
  std::cout << root_model->GetSubmodels().size() << std::endl;
  auto model_relation = root_model->GetModelRelation();
  ASSERT_TRUE(model_relation != nullptr);
  ASSERT_EQ(model_relation->submodel_queue_infos.size(), 2);
  ASSERT_EQ(model_relation->root_model_queue_info.input_queue_names.size(), 2);
  ASSERT_EQ(model_relation->root_model_queue_info.output_queue_names.size(), 1);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-1")->second.input_queue_names.size(), 2);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-1")->second.output_queue_names.size(), 1);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-2")->second.input_queue_names.size(), 2);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-2")->second.output_queue_names.size(), 1);

  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  auto ret = DeployPlanner(flow_model).BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  // data2 -> PC_1, data2 -> PC_2
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 10);
  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 6);
  std::map<int, std::set<int>> check;
  for (auto to_bind : deploy_plan.GetQueueBindings()) {
    check[to_bind.first].emplace(to_bind.second);
    std::cout << to_bind.first << " -> " << to_bind.second << std::endl;
  }
  ASSERT_EQ(check.size(), 3);
  ASSERT_EQ(check.begin()->second.size(), 4);
  ASSERT_TRUE(check.begin()->second.count(deploy_plan.submodels_["subgraph-1"].input_queue_indices[1]) > 0);
  ASSERT_TRUE(check.begin()->second.count(deploy_plan.submodels_["subgraph-1"].input_queue_indices[1]) > 0);

  ASSERT_EQ(deploy_plan.GetInputQueueIndices().size(), 2);
  ASSERT_EQ(deploy_plan.GetOutputQueueIndices().size(), 1);
}

///     NetOutput
///         |
///       PC_3
///      /   \.
///    PC_1  PC2
///    |      |
///  data1  data2
TEST_F(DeployPlannerTest, TestBuildDeployPlan_WithoutQueueBindings) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(root_model != nullptr);
  EXPECT_EQ(root_model->GetSubmodels().size(), 3);
  auto model_relation = root_model->GetModelRelation();
  ASSERT_TRUE(model_relation != nullptr);
  ASSERT_EQ(model_relation->submodel_queue_infos.size(), 3);
  ASSERT_EQ(model_relation->root_model_queue_info.input_queue_names.size(), 2);
  ASSERT_EQ(model_relation->root_model_queue_info.output_queue_names.size(), 1);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-1")->second.input_queue_names.size(), 1);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-1")->second.output_queue_names.size(), 1);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-2")->second.input_queue_names.size(), 1);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-2")->second.output_queue_names.size(), 1);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-3")->second.input_queue_names.size(), 2);
  ASSERT_EQ(model_relation->submodel_queue_infos.find("subgraph-3")->second.output_queue_names.size(), 1);

  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  auto ret = DeployPlanner(flow_model).BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 5);
  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 0);
}

TEST_F(DeployPlannerTest, TestGetQueueInfo) {
  DeployPlan plan;
  const DeployPlan::QueueInfo *queue_info = nullptr;
  ASSERT_EQ(plan.GetQueueInfo(0, queue_info), PARAM_INVALID);
  plan.queues_.resize(1);
  ASSERT_EQ(plan.GetQueueInfo(0, queue_info), SUCCESS);
}

TEST_F(DeployPlannerTest, TestAddingControlInput) {
  auto root_model = MakeShared<GeRootModel>();
  auto submodel_1 = MakeShared<GeRootModel>();
  submodel_1->SetModelName("subgraph-1");
  auto submodel_2 = MakeShared<GeRootModel>();
  submodel_2->SetModelName("subgraph-2");
  auto submodel_3 = MakeShared<GeRootModel>();
  submodel_3->SetModelName("subgraph-3");
  root_model->AddSubModel(submodel_1);
  root_model->AddSubModel(submodel_2);
  root_model->AddSubModel(submodel_3);

  root_model->model_relation_ = MakeShared<ModelRelation>();
  ModelRelation::QueueDef queue_def;
  queue_def.name = "ext_queue";
  queue_def.depth = 2;
  root_model->model_relation_->queue_defs.emplace_back(queue_def);
  queue_def.name = "out_1_0";
  root_model->model_relation_->queue_defs.emplace_back(queue_def);
  queue_def.name = "out_2_0";
  root_model->model_relation_->queue_defs.emplace_back(queue_def);
  queue_def.name = "out_3_0";
  root_model->model_relation_->queue_defs.emplace_back(queue_def);
  root_model->model_relation_->root_model_queue_info.external_queue_names = {"ext_queue"};
  root_model->model_relation_->root_model_queue_info.output_queue_names = {"out_1_0", "out_2_0", "out_3_0"};
  root_model->model_relation_->submodel_queue_infos["subgraph-1"].external_queue_names = {"ext_queue"};
  root_model->model_relation_->submodel_queue_infos["subgraph-1"].output_queue_names = {"out_1_0"};
  root_model->model_relation_->submodel_queue_infos["subgraph-2"].input_queue_names = {};
  root_model->model_relation_->submodel_queue_infos["subgraph-2"].output_queue_names = {"out_2_0"};
  root_model->model_relation_->submodel_queue_infos["subgraph-3"].input_queue_names = {};
  root_model->model_relation_->submodel_queue_infos["subgraph-3"].output_queue_names = {"out_3_0"};

  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  auto ret = DeployPlanner(flow_model).BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  std::cout << deploy_plan.GetAllInputQueueIndices().size()  << std::endl;
  ASSERT_EQ(deploy_plan.submodels_["subgraph-1"].input_queue_indices.size(), 0);
  ASSERT_EQ(deploy_plan.submodels_["subgraph-2"].input_queue_indices.size(), 0);
  ASSERT_EQ(deploy_plan.submodels_["subgraph-2"].control_input_queue_indices.size(), 1);
  ASSERT_EQ(deploy_plan.submodels_["subgraph-3"].input_queue_indices.size(), 0);
  ASSERT_EQ(deploy_plan.submodels_["subgraph-3"].control_input_queue_indices.size(), 1);
  ASSERT_EQ(deploy_plan.GetInputQueueIndices().size(), 0);
  ASSERT_EQ(deploy_plan.GetControlInputQueueIndices().size(), 1);
}
}  // namespace ge