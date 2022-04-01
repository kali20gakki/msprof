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
#include "graph/load/model_manager/task_info/cmo_task_info.h"
#include "graph/load/model_manager/davinci_model.h"

namespace ge {
class UtestCmoTaskInfo : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}

 public:
  void CreateCmoTaskDef(DavinciModel &davinci_model, domi::TaskDef &task_def) {
    rtStream_t stream = nullptr;
    rtStreamCreate(&stream, 0);
    davinci_model.stream_list_ = { stream };

    task_def.set_stream_id(0);
    domi::CmoTaskDef *cmo_task_def = task_def.mutable_cmo_task();
    cmo_task_def->set_cmo_type(1);
    cmo_task_def->set_logic_id(0);
    cmo_task_def->set_qos(0);
    cmo_task_def->set_part_id(0);
    cmo_task_def->set_pmg(0);
    cmo_task_def->set_op_code(0);
    cmo_task_def->set_num_inner(0);
    cmo_task_def->set_num_outer(0);
    cmo_task_def->set_length_inner(0);
    cmo_task_def->set_source_addr(0);
    cmo_task_def->set_strider_outer(0);
    cmo_task_def->set_strider_inner(0);

    davinci_model.runtime_param_.logic_mem_base = 0x8003000;
    davinci_model.runtime_param_.logic_weight_base = 0x8008000;
    davinci_model.runtime_param_.logic_var_base = 0x800e000;
    davinci_model.runtime_param_.mem_size = 0x5000;
    davinci_model.runtime_param_.weight_size = 0x6000;
    davinci_model.runtime_param_.var_size = 0x1000;
  }
};

TEST_F(UtestCmoTaskInfo, cmo_task_init_failed) {
  DavinciModel davinci_model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  davinci_model.stream_list_ = { stream };
  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  CmoTaskInfo cmo_task_info;
  EXPECT_EQ(cmo_task_info.Init(task_def, nullptr), PARAM_INVALID);
}

TEST_F(UtestCmoTaskInfo, cmo_task_init_success) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  CreateCmoTaskDef(davinci_model, task_def);
  CmoTaskInfo cmo_task_info;
  EXPECT_EQ(cmo_task_info.Init(task_def, &davinci_model), SUCCESS);
}

TEST_F(UtestCmoTaskInfo, success_cmo_task_distribute) {
  DavinciModel davinci_model(0, nullptr);
  domi::TaskDef task_def;
  CreateCmoTaskDef(davinci_model, task_def);
  CmoTaskInfo cmo_task_info;
  cmo_task_info.Init(task_def, &davinci_model);
  EXPECT_EQ(cmo_task_info.Distribute(), SUCCESS);
}
}