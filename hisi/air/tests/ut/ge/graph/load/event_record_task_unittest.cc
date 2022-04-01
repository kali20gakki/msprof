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

#define private public
#define protected public
#include "graph/load/model_manager/task_info/event_record_task_info.h"
#include "graph/load/model_manager/davinci_model.h"
#include "ut/ge/ffts_plus_proto_tools.h"

using namespace std;

namespace ge {
class UtestEventRecordTask : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

// test Init_EventRecordTaskInfo
TEST_F(UtestEventRecordTask, init_and_distribute_event_record_task_info) {
  domi::TaskDef task_def;
  task_def.set_stream_id(0);
  task_def.set_event_id(0);
  task_def.mutable_event_ex()->set_op_index(0);

  DavinciModel model(0, nullptr);
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };

  rtEvent_t event = nullptr;
  rtEventCreate(&event);
  model.event_list_ = { event };

  model.op_list_[0] = CreateOpDesc("op_name", "op_type");

  EventRecordTaskInfo task_info;
  EXPECT_EQ(task_info.Init(task_def, &model), SUCCESS);
  EXPECT_EQ(task_info.davinci_model_->GetOpByIndex(task_info.op_index_)->GetName(), "op_name");

  EXPECT_EQ(task_info.Distribute(), SUCCESS);
}

}  // namespace ge
