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
#include <gmock/gmock.h>
#include <vector>
#include <iostream>

#define private public
#define protected public
#include "hybrid/executor/hybrid_profiler.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;

class UtestHybridProfiling : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestHybridProfiling, test_basic_function) {
  HybridProfiler profiler;
  profiler.RecordEvent((HybridProfiler::EventType::GENERAL), "test");
  ASSERT_EQ(profiler.events_.empty(), false);
  ASSERT_EQ(profiler.events_.front().event_type, HybridProfiler::EventType::GENERAL);
  profiler.Dump(std::cout);
  ASSERT_EQ(profiler.counter_, 0);
}

}  // namespace ge