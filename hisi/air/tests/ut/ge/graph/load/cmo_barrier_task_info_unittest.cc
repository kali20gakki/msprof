/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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
#define private public
#define protected public
#include "graph/load/model_manager/task_info/cmo_barrier_task_info.h"
#include "graph/load/model_manager/davinci_model.h"

namespace ge {
class UtestCmoBarrierTaskInfo : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}

 public:
  void CreateCmoBarrierTaskDef(DavinciModel &davinci_model, domi::TaskDef &task_def) {
    rtStream_t stream = nullptr;
    rtStreamCreate(&stream, 0);
    davinci_model.stream_list_ = {stream};

    task_def.set_stream_id(0);
    domi::CmoBarrierTaskDef *cmo_barrier_task_def = task_def.mutable_cmo_barrier_task();
    cmo_barrier_task_def->set_logic_id_num(0);
    domi::CmoBarrierInfoDef *cmo_barrier_info_def = cmo_barrier_task_def->add_barrier_info();
    cmo_barrier_info_def->set_cmo_type(0);
    cmo_barrier_info_def->set_logic_id(0);
  }
};

TEST_F(UtestCmoBarrierTaskInfo, cmo_barrier_task_init_success) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  CreateCmoBarrierTaskDef(davinci_model, task_def);
  CmoBarrierTaskInfo cmo_barrier_task_info;
  EXPECT_EQ(cmo_barrier_task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestCmoBarrierTaskInfo, cmo_barrier_task_distribute_success) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  CreateCmoBarrierTaskDef(davinci_model, task_def);
  CmoBarrierTaskInfo cmo_barrier_task_info;
  cmo_barrier_task_info.Init(task_def, &davinci_model);
  EXPECT_EQ(cmo_barrier_task_info.Distribute(), SUCCESS);
}
}
