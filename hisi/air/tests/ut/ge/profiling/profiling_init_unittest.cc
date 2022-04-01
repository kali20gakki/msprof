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

#include <bits/stdc++.h>
#include <dirent.h>
#include <gtest/gtest.h>
#include <fstream>
#include <map>
#include <string>

#define protected public
#define private public
#include "common/profiling/profiling_init.h"
#include "graph/ge_local_context.h"
#include "graph/manager/graph_manager.h"
#include "common/profiling/profiling_properties.h"
#undef protected
#undef private

using namespace ge;
using namespace std;

namespace ge {
class UtestGeProfilingInit : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtestGeProfilingInit, Init) {
  Options opt;
  opt.profiling_mode = "1";
  opt.profiling_options = "profiling";
  auto &profiling_init = ge::ProfilingInit::Instance();
  EXPECT_EQ(profiling_init.Init(opt), PARAM_INVALID);
}

TEST_F(UtestGeProfilingInit, test_init) {
  setenv("PROFILING_MODE", "true", true);
  Options options;
  options.device_id = 0;
  options.job_id = "0";
  options.profiling_mode = "1";
  options.profiling_options = R"({"result_path":"/data/profiling","training_trace":"on","task_trace":"on","aicpu_trace":"on","fp_point":"Data_0","bp_point":"addn","ai_core_metrics":"ResourceConflictRatio"})";
  auto &profiling_init = ge::ProfilingInit::Instance();
  auto ret = profiling_init.Init(options);
  EXPECT_EQ(ret, ge::SUCCESS);
}

TEST_F(UtestGeProfilingInit, test_stop) {
  auto &profiling_init = ge::ProfilingInit::Instance();
  profiling_init.StopProfiling();
}

TEST_F(UtestGeProfilingInit, test_shut) {
  auto &prof_properties = ge::ProfilingProperties::Instance();
  prof_properties.SetExecuteProfiling(true);
  prof_properties.SetLoadProfiling(true);
  auto profiling_on = prof_properties.ProfilingOn();
  EXPECT_TRUE(profiling_on);
  auto &profiling_init = ge::ProfilingInit::Instance();
  profiling_init.ShutDownProfiling();
}

TEST_F(UtestGeProfilingInit, test_set_deviceId) {
  uint32_t model_id = 0;
  uint32_t device_id = 0;
  auto &profiling_init = ge::ProfilingInit::Instance();
  auto ret = profiling_init.SetDeviceIdByModelId(model_id, device_id);
}

TEST_F(UtestGeProfilingInit, test_unset_deviceId) {
  uint32_t model_id = 0;
  uint32_t device_id = 0;
  auto &profiling_init = ge::ProfilingInit::Instance();
  auto ret = profiling_init.UnsetDeviceIdByModelId(model_id, device_id);
}

TEST_F(UtestGeProfilingInit, GetProfilingModule) {
  uint32_t model_id = 0;
  uint32_t device_id = 0;
  auto &profiling_init = ge::ProfilingInit::Instance();
  EXPECT_NE(profiling_init.GetProfilingModule(), 0);
}

TEST_F(UtestGeProfilingInit, ParseOptions) {
  std::string options = "";
  auto &profiling_init = ge::ProfilingInit::Instance();
  EXPECT_EQ(profiling_init.ParseOptions(options), PARAM_INVALID);
  options = "json";
  EXPECT_EQ(profiling_init.ParseOptions(options), PARAM_INVALID);
  options = "{\"json\":\"value\"}";
  EXPECT_EQ(profiling_init.ParseOptions(options), SUCCESS);
  options = "{\"training_trace\":\"off\"}";
  EXPECT_EQ(profiling_init.ParseOptions(options), PARAM_INVALID);
}

TEST_F(UtestGeProfilingInit, InitFromOptions) {
  Options opt;
  opt.profiling_mode = "1";
  opt.profiling_options = "invalid";
  MsprofGeOptions meo{};
  bool is_execute_profiling = false;
  auto &profiling_init = ge::ProfilingInit::Instance();
  EXPECT_EQ(profiling_init.InitFromOptions(opt, meo, is_execute_profiling), PARAM_INVALID);

  const char* default_options = "{\"output\":\"./\",\"training_trace\":\"on\",\"task_trace\":\"on\","\
        "\"hccl\":\"on\",\"aicpu\":\"on\",\"aic_metrics\":\"PipeUtilization\",\"msproftx\":\"off\"}";
  struct MsprofGeOptions prof_conf = {};
  Options options;
  char_t npu_collect_path[MMPA_MAX_PATH] = "true";
  mmSetEnv("PROFILING_MODE", &npu_collect_path[0U], MMPA_MAX_PATH);
  Status ret = profiling_init.InitFromOptions(options, prof_conf, is_execute_profiling);
  EXPECT_EQ(strcmp(prof_conf.options, default_options), 0);
  unsetenv("PROFILING_MODE");
}


}  // namespace ge