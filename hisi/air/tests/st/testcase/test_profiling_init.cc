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
#include "common/profiling/profiling_init.h"
#include "graph/ge_local_context.h"
#include "graph/manager/graph_manager.h"
#include "common/profiling/profiling_properties.h"
#undef protected
#undef private

namespace ge {
class ProfilingInitTest : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(ProfilingInitTest, test_shut) {
  auto &prof_properties = ge::ProfilingProperties::Instance();
  prof_properties.SetExecuteProfiling(true);
  prof_properties.SetLoadProfiling(true);
  auto profiling_on = prof_properties.ProfilingOn();
  EXPECT_TRUE(profiling_on);
  char_t npu_collect_path[MMPA_MAX_PATH] = "true";
  mmSetEnv("PROFILING_MODE", &npu_collect_path[0U], MMPA_MAX_PATH);
  Options options;
  options.device_id = 0;
  options.job_id = "0";
  options.profiling_mode = "1";
  options.profiling_options = R"({"result_path":"/data/profiling","training_trace":"on","task_trace":"on","aicpu_trace":"on","fp_point":"Data_0","bp_point":"addn","ai_core_metrics":"ResourceConflictRatio"})";
  auto &profiling_init = ge::ProfilingInit::Instance();
  auto ret = profiling_init.Init(options);
  EXPECT_EQ(ret, ge::SUCCESS);
  profiling_init.ShutDownProfiling();
  unsetenv("PROFILING_MODE");
}
}  // namespace ge
