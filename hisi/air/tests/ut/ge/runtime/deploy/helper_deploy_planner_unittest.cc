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
#include "deploy/deployer/helper_deploy_planner.h"
#undef private
#undef protected

#include "graph/passes/graph_builder_utils.h"
#include "graph/build/graph_builder.h"
#include "runtime/deploy/stub_models.h"

using namespace std;

namespace ge {
class HelperDeployPlannerTest : public testing::Test {
 protected:
  void SetUp() override {
  }
  void TearDown() override {
  }

  std::vector<DeployPlan::DeviceInfo>
      double_device_list{DeployPlan::DeviceInfo{0, 1, 0}, DeployPlan::DeviceInfo{0, 1, 1}};
  std::vector<DeployPlan::DeviceInfo> single_device_list{DeployPlan::DeviceInfo{0, 1, 0}};
};

///      NetOutput
///         |
///         |
///        PC_2
///        |  \
///       PC_1 |
///     /    \
///    |      |
///  data1  data2
TEST_F(HelperDeployPlannerTest, TestBuildDeployPlan) {
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
  std::vector<GeRootModelPtr> models;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  auto ret = HelperDeployPlanner(flow_model, single_device_list).BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  // data2 -> PC_1, data2 -> PC_2
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 16);
  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 12);
  ASSERT_EQ(deploy_plan.GetInputQueueIndices().size(), 2);
  ASSERT_EQ(deploy_plan.GetOutputQueueIndices().size(), 1);
}

///     NetOutput
///         |
///       PC_3
///      /   \
///    PC_1  PC2
///    |      |
///  data1  data2
TEST_F(HelperDeployPlannerTest, TestBuildDeployPlan_2) {
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
  std::vector<GeRootModelPtr> models;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  auto ret = HelperDeployPlanner(flow_model, single_device_list).BuildPlan(deploy_plan);

  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 14);
  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 9);
}

///      NetOutput
///         |
///         |
///       XXXX
///     /     \
///    |      |
///  data1  data2
TEST_F(HelperDeployPlannerTest, TestBuildPlanForSingleModel) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithQueueBindings());
  ASSERT_TRUE(root_model != nullptr);
  root_model->submodels_.clear();
  root_model->model_relation_.reset();
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  HelperDeployPlanner planner(flow_model, single_device_list);
  DeployPlan deploy_plan;
  auto ret = planner.BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 12);
  ASSERT_EQ(deploy_plan.GetInputQueueIndices().size(), 2);
  ASSERT_EQ(deploy_plan.GetOutputQueueIndices().size(), 1);

  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 9);
  ASSERT_EQ(deploy_plan.GetSubmodels().size(), 1);
  auto iter = deploy_plan.submodels_.begin();
  auto &submodel_info = deploy_plan.submodels_[iter->first];
  ASSERT_EQ(submodel_info.input_queue_indices.size(), deploy_plan.GetInputQueueIndices().size());
  ASSERT_EQ(submodel_info.output_queue_indices.size(), deploy_plan.GetOutputQueueIndices().size());
}


TEST_F(HelperDeployPlannerTest, TestFailedDueToTheAbsenceOfModel) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithQueueBindings());
  ASSERT_TRUE(root_model != nullptr);
  root_model->submodels_.clear();
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  HelperDeployPlanner planner(flow_model, single_device_list);
  DeployPlan deploy_plan;
  auto ret = planner.BuildPlan(deploy_plan);
  ASSERT_EQ(ret, PARAM_INVALID);
}


void AddQueueDef(ModelRelation &model_relation, const std::string &name) {
  ModelRelation::QueueDef queue_def;
  queue_def.name = name;
  model_relation.queue_defs.emplace_back(queue_def);
}

///         NetOutput
///            |
///        Submodel-3
///       /         \
///    Submodel-1  Submodel-2
///     |     |     |   \
///    D1    D2    D3   D4
TEST_F(HelperDeployPlannerTest, TestBuildDeployPlan_AllRawModel) {
  auto submodel_1 = std::make_shared<GeRootModel>();
  auto submodel_2 = std::make_shared<GeRootModel>();
  auto submodel_3 = std::make_shared<GeRootModel>();
  ASSERT_TRUE(submodel_1 != nullptr);
  ASSERT_TRUE(submodel_2 != nullptr);
  ASSERT_TRUE(submodel_3 != nullptr);
  submodel_1->SetModelName("submodel-1");
  submodel_2->SetModelName("submodel-2");
  submodel_3->SetModelName("submodel-3");

  auto model_relation_ptr = std::make_shared<ModelRelation>();
  ASSERT_TRUE(model_relation_ptr != nullptr);
  ModelRelation &model_relation = *model_relation_ptr;
  model_relation.root_model_queue_info.input_queue_names = {"in-queue-1", "in-queue-2", "in-queue-3", "in-queue-4"};
  model_relation.root_model_queue_info.output_queue_names = {"out-queue-1"};
  model_relation.submodel_queue_infos["submodel-1"].input_queue_names = {"in-queue-1", "in-queue-2"};
  model_relation.submodel_queue_infos["submodel-1"].output_queue_names = {"inner-queue-1"};
  model_relation.submodel_queue_infos["submodel-2"].input_queue_names = {"in-queue-3", "in-queue-4"};
  model_relation.submodel_queue_infos["submodel-2"].output_queue_names = {"inner-queue-2"};
  model_relation.submodel_queue_infos["submodel-3"].input_queue_names = {"inner-queue-1", "inner-queue-2"};
  model_relation.submodel_queue_infos["submodel-3"].output_queue_names = {"out-queue-1"};
  AddQueueDef(model_relation, "in-queue-1");
  AddQueueDef(model_relation, "in-queue-2");
  AddQueueDef(model_relation, "in-queue-3");
  AddQueueDef(model_relation, "in-queue-4");
  AddQueueDef(model_relation, "out-queue-1");
  AddQueueDef(model_relation, "inner-queue-1");
  AddQueueDef(model_relation, "inner-queue-2");

  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(submodel_1);
  flow_model->AddSubModel(submodel_2);
  flow_model->AddSubModel(submodel_3);
  flow_model->SetModelRelation(model_relation_ptr);
  HelperDeployPlanner planner(flow_model, single_device_list);
  auto ret = planner.BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 22);
  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 15);
  ASSERT_EQ(deploy_plan.submodels_["submodel-1@0_1_0"].model, submodel_1);
  ASSERT_EQ(deploy_plan.submodels_["submodel-2@0_1_0"].model, submodel_2);
  ASSERT_EQ(deploy_plan.submodels_["submodel-3@0_1_0"].model, submodel_3);
  ASSERT_EQ(deploy_plan.GetSubmodels().size(), 3);
}

///     NetOutput
///         |
///       PC_3
///      /   \         -> PC_4 2-inputs, 1-output
///    PC_1  PC2
///    |      |
///  data1  data2
///
///
///       NetOutput
///          |
///        PC_4_3
///       /     \
///    PC_4_1  PC_4_2
///     |  |    |  \
///    D1 D2   D3  D4
TEST_F(HelperDeployPlannerTest, TestBuildDeployPlan_AllWrappedModels) {
  auto submodel_1 = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  auto submodel_2 = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  auto submodel_3 = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(submodel_1 != nullptr);
  ASSERT_TRUE(submodel_2 != nullptr);
  ASSERT_TRUE(submodel_3 != nullptr);
  submodel_1->SetModelName("submodel-1");
  submodel_2->SetModelName("submodel-2");
  submodel_3->SetModelName("submodel-3");
  EXPECT_EQ(submodel_1->GetSubmodels().size(), 3);

  auto model_relation_ptr = std::make_shared<ModelRelation>();
  ASSERT_TRUE(model_relation_ptr != nullptr);
  ModelRelation &model_relation = *model_relation_ptr;
  model_relation.root_model_queue_info.input_queue_names = {"in-queue-1", "in-queue-2", "in-queue-3", "in-queue-4"};
  model_relation.root_model_queue_info.output_queue_names = {"out-queue-1"};
  model_relation.submodel_queue_infos["submodel-1"].input_queue_names = {"in-queue-1", "in-queue-2"};
  model_relation.submodel_queue_infos["submodel-1"].output_queue_names = {"inner-queue-1"};
  model_relation.submodel_queue_infos["submodel-2"].input_queue_names = {"in-queue-3", "in-queue-4"};
  model_relation.submodel_queue_infos["submodel-2"].output_queue_names = {"inner-queue-2"};
  model_relation.submodel_queue_infos["submodel-3"].input_queue_names = {"inner-queue-1", "inner-queue-2"};
  model_relation.submodel_queue_infos["submodel-3"].output_queue_names = {"out-queue-1"};
  AddQueueDef(model_relation, "in-queue-1");
  AddQueueDef(model_relation, "in-queue-2");
  AddQueueDef(model_relation, "in-queue-3");
  AddQueueDef(model_relation, "in-queue-4");
  AddQueueDef(model_relation, "out-queue-1");
  AddQueueDef(model_relation, "inner-queue-1");
  AddQueueDef(model_relation, "inner-queue-2");

  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(submodel_1);
  flow_model->AddSubModel(submodel_2);
  flow_model->AddSubModel(submodel_3);
  flow_model->SetModelRelation(model_relation_ptr);
  HelperDeployPlanner planner(flow_model, single_device_list);
  auto ret = planner.BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 28);
  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 15);
  ASSERT_EQ(deploy_plan.GetSubmodels().size(), 9);
}

///     NetOutput
///         |
///       PC_3
///      /   \         -> PC_4 2-inputs, 1-output
///    PC_1  PC2
///    |      |
///  data1  data2
///
///
///       NetOutput
///          |
///        PC_4_3
///       /     \
///    PC_4_1  Submodel-2
///     |  |    |  \
///    D1 D2   D3  D4
TEST_F(HelperDeployPlannerTest, TestBuildDeployPlan_Mixed) {
  auto submodel_1 = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  auto submodel_2 = std::make_shared<GeRootModel>();  // single model
  auto submodel_3 = SubModels::BuildRootModel(SubModels::BuildGraphWithoutNeedForBindingQueues());
  ASSERT_TRUE(submodel_1 != nullptr);
  ASSERT_TRUE(submodel_2 != nullptr);
  ASSERT_TRUE(submodel_3 != nullptr);
  submodel_1->SetModelName("submodel-1");
  submodel_2->SetModelName("submodel-2");
  submodel_3->SetModelName("submodel-3");
  EXPECT_EQ(submodel_1->GetSubmodels().size(), 3);

  auto model_relation_ptr = std::make_shared<ModelRelation>();
  ASSERT_TRUE(model_relation_ptr != nullptr);
  ModelRelation &model_relation = *model_relation_ptr;
  model_relation.root_model_queue_info.input_queue_names = {"in-queue-1", "in-queue-2", "in-queue-3", "in-queue-4"};
  model_relation.root_model_queue_info.output_queue_names = {"out-queue-1"};
  model_relation.submodel_queue_infos["submodel-1"].input_queue_names = {"in-queue-1", "in-queue-2"};
  model_relation.submodel_queue_infos["submodel-1"].output_queue_names = {"inner-queue-1"};
  model_relation.submodel_queue_infos["submodel-2"].input_queue_names = {"in-queue-3", "in-queue-4"};
  model_relation.submodel_queue_infos["submodel-2"].output_queue_names = {"inner-queue-2"};
  model_relation.submodel_queue_infos["submodel-3"].input_queue_names = {"inner-queue-1", "inner-queue-2"};
  model_relation.submodel_queue_infos["submodel-3"].output_queue_names = {"out-queue-1"};
  AddQueueDef(model_relation, "in-queue-1");
  AddQueueDef(model_relation, "in-queue-2");
  AddQueueDef(model_relation, "in-queue-3");
  AddQueueDef(model_relation, "in-queue-4");
  AddQueueDef(model_relation, "out-queue-1");
  AddQueueDef(model_relation, "inner-queue-1");
  AddQueueDef(model_relation, "inner-queue-2");

  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(submodel_1);
  flow_model->AddSubModel(submodel_2);
  flow_model->AddSubModel(submodel_3);
  flow_model->SetModelRelation(model_relation_ptr);
  HelperDeployPlanner planner(flow_model, single_device_list);
  auto ret = planner.BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 26);
  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 15);
  ASSERT_EQ(deploy_plan.GetSubmodels().size(), 7);
  ASSERT_EQ(deploy_plan.submodels_["submodel-2@0_1_0"].model, submodel_2);
}

TEST_F(HelperDeployPlannerTest, TestBuildDeployPlan_NoModel) {
  DeployPlan deploy_plan;
  ASSERT_EQ(HelperDeployPlanner(nullptr, {DeployPlan::DeviceInfo{0, 0, 0}}).BuildPlan(deploy_plan), PARAM_INVALID);
}

TEST_F(HelperDeployPlannerTest, TestBuildDeployPlan_MultiplyModelWithNoRelation) {
  auto submodel_1 = std::make_shared<GeRootModel>();
  auto submodel_2 = std::make_shared<GeRootModel>();
  DeployPlan deploy_plan;
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(submodel_1);
  flow_model->AddSubModel(submodel_2);
  ASSERT_EQ(HelperDeployPlanner(flow_model, {DeployPlan::DeviceInfo{0, 0, 0}}).BuildPlan(deploy_plan), PARAM_INVALID);
}

///  NetOutput
///      |
///      |
///     XXXX
///     /  \
///    |    |
///  data1  data2
TEST_F(HelperDeployPlannerTest, TestBuildPlanForSingleModel_2PG) {
  auto root_model = SubModels::BuildRootModel(SubModels::BuildGraphWithQueueBindings());
  ASSERT_TRUE(root_model != nullptr);
  root_model->submodels_.clear();
  root_model->model_relation_.reset();
  auto flow_model = std::make_shared<FlowModel>();
  ASSERT_TRUE(flow_model != nullptr);
  flow_model->AddSubModel(root_model);
  HelperDeployPlanner planner(flow_model, double_device_list);
  DeployPlan deploy_plan;
  auto ret = planner.BuildPlan(deploy_plan);
  ASSERT_EQ(ret, SUCCESS);
  ASSERT_EQ(deploy_plan.GetQueueInfoList().size(), 18);
  ASSERT_EQ(deploy_plan.GetInputQueueIndices().size(), 2);
  ASSERT_EQ(deploy_plan.GetOutputQueueIndices().size(), 1);

  ASSERT_EQ(deploy_plan.GetQueueBindings().size(), 15);
  ASSERT_EQ(deploy_plan.GetSubmodels().size(), 2);
  ASSERT_EQ(deploy_plan.GetGroups().size(), 9);
}
}  // namespace ge
