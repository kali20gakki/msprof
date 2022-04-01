/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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

#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/task_info/profiler_trace_task_info.h"

namespace ge {

class UtestProfilerTraceTaskInfo : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {}
};

// test ProfilerTraceTaskInfo Init.
TEST_F(UtestProfilerTraceTaskInfo, task_init_infer) {
  DavinciModel model(0, nullptr);
  model.SetId(1);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_PROFILER_TRACE);
  ProfilerTraceTaskInfo task_info;
  task->stream_id_ = 0;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };

  EXPECT_EQ(task_info.Init(*task, &model), SUCCESS);
}

TEST_F(UtestProfilerTraceTaskInfo, test_init_train) {
  DavinciModel model(0, nullptr);
  model.SetId(1);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_PROFILER_TRACE);
  ProfilerTraceTaskInfo task_info;
  domi::GetContext().is_online_model = true;
  task->stream_id_ = 0;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };
  EXPECT_EQ(task_info.Init(*task, &model), SUCCESS);
}

// test ProfilerTraceTaskInfo Distribute.
TEST_F(UtestProfilerTraceTaskInfo, test_distribute) {
  DavinciModel model(0, nullptr);
  model.SetId(1);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_PROFILER_TRACE);
  ProfilerTraceTaskInfo task_info;
  domi::GetContext().is_online_model = false;
  task->stream_id_ = 0;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };
  task_info.Init(*task, &model);
  EXPECT_EQ(task_info.Distribute(), SUCCESS);
}

TEST_F(UtestProfilerTraceTaskInfo, test_distribute_out_range) {
  DavinciModel model(0, nullptr);
  model.SetId(1);
  domi::ModelTaskDef model_task_def;
  domi::TaskDef *task = model_task_def.add_task();
  task->set_type(RT_MODEL_TASK_PROFILER_TRACE);
  ProfilerTraceTaskInfo task_info;
  task->stream_id_ = 0;
  rtStream_t stream = nullptr;
  rtStreamCreate(&stream, 0);
  model.stream_list_ = { stream };
  task_info.Init(*task, &model);
  task_info.log_id_ = 20000;
  EXPECT_EQ(task_info.Distribute(), SUCCESS);
}
}  // namespace ge
