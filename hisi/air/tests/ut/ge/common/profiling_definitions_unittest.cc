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
#include "common/profiling_definitions.h"

namespace ge {
class ProfilingUt : public testing::Test {};

TEST_F(ProfilingUt, DefaultRegisteredString) {
  profiling::ProfilingContext pc;
  EXPECT_EQ(strcmp(pc.GetProfiler()->GetStringHashes()[profiling::kInferShape].str, "InferShape"), 0);
  EXPECT_EQ(strcmp(pc.GetProfiler()->GetStringHashes()[profiling::kAllocMem].str, "AllocMemory"), 0);
  for (auto i = 0; i < profiling::kProfilingIndexEnd; ++i) {
    EXPECT_NE(strlen(pc.GetProfiler()->GetStringHashes()[i].str), 0);
  }
  EXPECT_EQ(strlen(pc.GetProfiler()->GetStringHashes()[profiling::kProfilingIndexEnd].str), 0);
}

TEST_F(ProfilingUt, RegisteredStringHash) {
  profiling::ProfilingContext pc;
  const std::string &str = __FUNCTION__;
  const uint64_t test_hash_id = 0x5aU;
  const int64_t idx = pc.RegisterStringHash(test_hash_id, str);
  EXPECT_EQ(idx != 0, true);
}

TEST_F(ProfilingUt, RecordOne) {
  profiling::ProfilingContext::GetInstance().SetEnable();
  PROFILING_START(-1, profiling::kInferShape);
  PROFILING_END(-1, profiling::kInferShape);
  EXPECT_EQ(profiling::ProfilingContext::GetInstance().GetProfiler()->GetRecordNum(), 2);
}

TEST_F(ProfilingUt, RegisterStringBeforeRecord) {
  auto index = profiling::ProfilingContext::GetInstance().RegisterString("Data");
  EXPECT_EQ(strcmp(profiling::ProfilingContext::GetInstance().GetProfiler()->GetStringHashes()[index].str, "Data"), 0);
}

TEST_F(ProfilingUt, DumpWhenDisable) {
  profiling::ProfilingContext::GetInstance().Reset();
  profiling::ProfilingContext::GetInstance().SetDisable();
  PROFILING_START(-1, profiling::kInferShape);
  PROFILING_END(-1, profiling::kInferShape);
  EXPECT_EQ(profiling::ProfilingContext::GetInstance().GetProfiler()->GetRecordNum(), 0);
  std::stringstream ss;
  profiling::ProfilingContext::GetInstance().Dump(ss);
  profiling::ProfilingContext::GetInstance().Reset();
  EXPECT_EQ(ss.str(), std::string("Profiling not enable, skip to dump\n"));
}
TEST_F(ProfilingUt, ScopeRecordOk) {
  profiling::ProfilingContext::GetInstance().SetEnable();
  {
    PROFILING_SCOPE(-1, profiling::kInferShape);
  }
  EXPECT_EQ(profiling::ProfilingContext::GetInstance().GetProfiler()->GetRecordNum(), 2);
}

TEST_F(ProfilingUt, UpdateElementHashId) {
  profiling::ProfilingContext pc;
  static uint32_t record_profiling_num = 0UL;
  MsprofReporterCallback func = [](uint32_t a, uint32_t b, void *c, uint32_t d)->int32_t {
    record_profiling_num++;
    return 0;
  };
  pc.UpdateElementHashId(func);
  EXPECT_EQ(record_profiling_num, profiling::kProfilingIndexEnd);
}

/* just for coverage */
TEST_F(ProfilingUt, IsDumpToStdEnabled) {
  EXPECT_TRUE(profiling::ProfilingContext::GetInstance().IsDumpToStdEnabled());
}
TEST_F(ProfilingUt, DumpToStdOut) {
  profiling::ProfilingContext::GetInstance().SetEnable();
  profiling::ProfilingContext::GetInstance().DumpToStdOut();
}
}
