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
#include "common/profiling/profiling_properties.h"
#include "graph/ge_local_context.h"
#include "graph/manager/graph_manager.h"
#undef protected
#undef private

using namespace ge;
using namespace std;

class UtestGeProfilingProperties : public testing::Test {
 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtestGeProfilingProperties, test_execute_profiling) {
  auto &profiling_properties = ge::ProfilingProperties::Instance();
  profiling_properties.SetExecuteProfiling(true);
  auto is_execute = profiling_properties.IsExecuteProfiling();
  EXPECT_EQ(is_execute, true);
}

TEST_F(UtestGeProfilingProperties, test_training_trace) {
  auto &profiling_properties = ge::ProfilingProperties::Instance();
  profiling_properties.SetTrainingTrace(true);
  auto is_train_trance = profiling_properties.ProfilingTrainingTraceOn();
  EXPECT_EQ(is_train_trance, true);
}

TEST_F(UtestGeProfilingProperties, test_fpbp_point) {
auto &profiling_properties = ge::ProfilingProperties::Instance();
  std::string fp_point = "fp";
  std::string bp_point = "bp";
  profiling_properties.SetFpBpPoint(fp_point, bp_point);
  profiling_properties.GetFpBpPoint(fp_point, bp_point);
  EXPECT_EQ(fp_point, "fp");
  EXPECT_EQ(bp_point, "bp");
}

TEST_F(UtestGeProfilingProperties, test_profiling_on) {
  auto &profiling_properties = ge::ProfilingProperties::Instance();
  profiling_properties.SetExecuteProfiling(true);
  profiling_properties.SetLoadProfiling(true);
  auto profiling_on = profiling_properties.ProfilingOn();
  EXPECT_EQ(profiling_on, true);
}
