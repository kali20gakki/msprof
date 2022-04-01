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

#include "common/opskernel/ops_kernel_info_store.h"
#include "depends/mmpa/src/mmpa_stub.h"

#define private public
#define protected public
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/task_info/hccl_task_info.h"
#include "opskernel_executor/ops_kernel_executor_manager.h"

namespace ge {
namespace {
int32_t g_unload_called_count = 0;

class HcclOpsKernelInfoStore : public OpsKernelInfoStore {
 public:
  HcclOpsKernelInfoStore() = default;
  Status Initialize(const std::map<std::string, std::string> &options) override { return SUCCESS; }
  // close opsKernelInfoStore
  Status Finalize() override { return SUCCESS; }
  // get all opsKernelInfo
  void GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const override {}
  // whether the opsKernelInfoStore is supported based on the operator attribute
  bool CheckSupported(const OpDescPtr &opDescPtr, std::string &un_supported_reason) const override { return true; }
  Status UnloadTask(GETaskInfo &task) {
    g_unload_called_count++;
    return SUCCESS;
  }
};
class FailHcclOpsKernelInfoStore : public OpsKernelInfoStore {
 public:
  FailHcclOpsKernelInfoStore() = default;
  Status Initialize(const std::map<std::string, std::string> &options) override { return SUCCESS; }
  // close opsKernelInfoStore
  Status Finalize() override { return SUCCESS; }
  // get all opsKernelInfo
  void GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const override {}
  // whether the opsKernelInfoStore is supported based on the operator attribute
  bool CheckSupported(const OpDescPtr &opDescPtr, std::string &un_supported_reason) const override { return true; }
  Status LoadTask(GETaskInfo &task) { return FAILED; }
  Status UnloadTask(GETaskInfo &task) {
    g_unload_called_count++;
    return FAILED;
  }
};
}  // namespace

class UtestHcclTaskInfo : public testing::Test {
 protected:
  void SetUp() { g_unload_called_count = 0; }
  void TearDown() {}
};

// test success GetTaskID
TEST_F(UtestHcclTaskInfo, success_get_task_id) {
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_KERNEL);
  TaskInfoPtr task_info = TaskInfoFactory::Instance().Create(static_cast<rtModelTaskType_t>(task->type()));

  EXPECT_EQ(task_info->GetTaskID(), 0);

  HcclTaskInfo hccl_task_info;
  EXPECT_EQ(hccl_task_info.GetTaskID(), 0);
}

TEST_F(UtestHcclTaskInfo, test_SetFollowStream) {
  DavinciModel model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.main_follow_stream_mapping_[0].emplace_back(stream);

  HcclTaskInfo hccl_task_info;
  auto op_desc = std::make_shared<OpDesc>("hcom_reduce", HCOMREDUCE);
  AttrUtils::SetInt(op_desc, "used_stream_num", 1);
  hccl_task_info.davinci_model_ = &model;
  EXPECT_EQ(hccl_task_info.SetFollowStream(op_desc), SUCCESS);

  AttrUtils::SetInt(op_desc, "used_stream_num", 2);
  EXPECT_EQ(hccl_task_info.SetFollowStream(op_desc), SUCCESS);
}

// test hccl_init
TEST_F(UtestHcclTaskInfo, success_task_init) {
  DavinciModel model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = {stream};

  domi::TaskDef task_def;
  domi::KernelHcclDef *kernel_hccl_def = task_def.mutable_kernel_hccl();
  kernel_hccl_def->set_op_index(0);
  kernel_hccl_def->set_hccl_type("HcomAllReduce");
  GeTensorDesc desc;
  auto op_desc = std::make_shared<OpDesc>("hcom_reduce", HCOMREDUCE);
  AttrUtils::SetInt(op_desc, HCOM_ATTR_ROOT_RANK, 0);
  AttrUtils::SetStr(op_desc, HCOM_ATTR_REDUCE_TYPE, "min");
  op_desc->SetStreamId(0);
  op_desc->SetId(0);
  op_desc->AddInputDesc(desc);
  op_desc->SetInputOffset({8});
  op_desc->SetWorkspaceBytes({150});
  model.op_list_[op_desc->GetId()] = op_desc;
  HcclTaskInfo hccl_task_info;
  EXPECT_EQ(hccl_task_info.Init(task_def, &model), SUCCESS);

  domi::TaskDef task_def1;
  domi::KernelHcclDef *kernel_hccl_def1 = task_def1.mutable_kernel_hccl();
  kernel_hccl_def1->set_op_index(1);
  auto op_desc1 = std::make_shared<OpDesc>("hvd_wait", HVDWAIT);
  op_desc1->SetStreamId(0);
  op_desc1->SetId(1);
  model.op_list_[op_desc1->GetId()] = op_desc1;
  HcclTaskInfo hccl_task_info1;
  EXPECT_EQ(hccl_task_info1.Init(task_def1, &model), SUCCESS);

  model.known_node_ = true;
  HcclTaskInfo hccl_task_info2;
  EXPECT_EQ(hccl_task_info2.Init(task_def, &model), SUCCESS);
  task_def.clear_kernel_hccl();
  task_def1.clear_kernel_hccl();
}

// test hccl_init
TEST_F(UtestHcclTaskInfo, fail_task_init) {
  DavinciModel model(0, nullptr);
  domi::TaskDef task_def;
  HcclTaskInfo hccl_task_info;
  EXPECT_EQ(hccl_task_info.Init(task_def, &model), FAILED);

  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = {stream};

  domi::KernelHcclDef *kernel_hccl_def = task_def.mutable_kernel_hccl();
  kernel_hccl_def->set_op_index(0);
  kernel_hccl_def->set_hccl_type("HcomAllReduce");
  auto op_desc = std::make_shared<OpDesc>("hcom_reduce", HCOMREDUCE);
  op_desc->SetStreamId(0);
  op_desc->SetId(0);
  op_desc->SetInputOffset({8});
  op_desc->SetWorkspaceBytes({150});
  model.op_list_[op_desc->GetId()] = op_desc;

  // fail for GetHcclDataType
  HcclTaskInfo hccl_task_info1;
  EXPECT_EQ(hccl_task_info1.Init(task_def, &model), PARAM_INVALID);

  // fail for GetAllRootId
  GeTensorDesc desc;
  op_desc->AddInputDesc(desc);
  HcclTaskInfo hccl_task_info2;
  EXPECT_EQ(hccl_task_info2.Init(task_def, &model), FAILED);

  // fail for SetAddrs
  AttrUtils::SetInt(op_desc, HCOM_ATTR_ROOT_RANK, 0);
  HcclTaskInfo hccl_task_info3;
  EXPECT_EQ(hccl_task_info3.Init(task_def, &model), PARAM_INVALID);

  task_def.clear_kernel_hccl();
}

// test hccl_GetPrivateDefByTaskDef
TEST_F(UtestHcclTaskInfo, success_hccl_get_private_def_by_task_def) {
  DavinciModel model(0, nullptr);
  model.op_list_[0] = std::make_shared<OpDesc>("FrameworkOp", "FrameworkOp");

  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task7 = model_task_def.add_task();
  domi::KernelHcclDef *kernel_hccl_def = task7->mutable_kernel_hccl();
  kernel_hccl_def->set_op_index(0);
  task7->set_type(RT_MODEL_TASK_HCCL);
  // for SetStream
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_.push_back(stream);
  // for GetPrivateDefByTaskDef
  std::string value = "hccl_task";
  task7->set_private_def(value);

  TaskInfoPtr task_info7 = TaskInfoFactory::Instance().Create(static_cast<rtModelTaskType_t>(task7->type()));

  OpsKernelExecutorManager::GetInstance().Initialize({});
  OpsKernelExecutor *p_executor = nullptr;
  OpsKernelExecutorManager::GetInstance().executors_[kEngineNameHccl] = std::make_shared<HcclOpsKernelInfoStore>();
  OpsKernelExecutorManager::GetInstance().GetExecutor(kEngineNameHccl, p_executor);
  EXPECT_TRUE(p_executor != nullptr);
  // for Distribute
  EXPECT_EQ(task_info7->Init(task7[0], &model), SUCCESS);
  OpsKernelExecutorManager::GetInstance().executors_.clear();
}

TEST_F(UtestHcclTaskInfo, test_hccl_task_distribute_release) {
  auto kernel_info_store = std::make_shared<HcclOpsKernelInfoStore>();
  HcclTaskInfo hccl_task_info;
  // without ops_kernel_info_store, fail
  EXPECT_EQ(hccl_task_info.Distribute(), INTERNAL_ERROR);
  EXPECT_EQ(hccl_task_info.Release(), SUCCESS);

  // ops_kernel_info_store return success
  hccl_task_info.ops_kernel_store_ = kernel_info_store.get();
  EXPECT_EQ(hccl_task_info.Distribute(), SUCCESS);
  EXPECT_EQ(hccl_task_info.Release(), SUCCESS);
  ASSERT_EQ(g_unload_called_count, 0);

  // ops_kernel_info_store return failed
  auto fail_kernel_info_store = std::make_shared<FailHcclOpsKernelInfoStore>();
  hccl_task_info.ops_kernel_store_ = fail_kernel_info_store.get();
  EXPECT_EQ(hccl_task_info.Distribute(), INTERNAL_ERROR);
  EXPECT_EQ(hccl_task_info.Release(), SUCCESS);
  ASSERT_EQ(g_unload_called_count, 0);
}

TEST_F(UtestHcclTaskInfo, Calculate_Update_Args) {
  DavinciModel model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = {stream};
  model.op_list_[0] = std::make_shared<OpDesc>("FrameworkOp", "FrameworkOp");

  domi::TaskDef task_def;
  domi::KernelHcclDef *kernel_hccl_def = task_def.mutable_kernel_hccl();
  kernel_hccl_def->set_op_index(0);
  kernel_hccl_def->set_hccl_type("HcomBroadcast");

  HcclTaskInfo hccl_task_info;
  hccl_task_info.Init(task_def, &model);

  auto ret = hccl_task_info.CalculateArgs(task_def, &model);
  EXPECT_EQ(ret, SUCCESS);

  ret = hccl_task_info.UpdateArgs();
  EXPECT_EQ(ret, SUCCESS);

  task_def.clear_kernel_hccl();
}
}  // namespace ge
