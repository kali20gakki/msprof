/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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
#include <map>

#define private public
#define protected public
#include "hybrid/node_executor/ffts_plus/ffts_plus_sub_task_factory.h"

using namespace std;
using namespace testing;

namespace ge {
using namespace hybrid;

const std::string kCoreTypeTest = "AIC_AIV";
REGISTER_FFTS_PLUS_CTX_UPDATER(kCoreTypeTest, FFTSPlusTaskUpdate);
REGISTER_FFTS_PLUS_SUB_TASK_CREATOR(kCoreTypeTest, FftsPlusAiCoreTask);

class UtestFftsPlusSubTaskFactory : public testing::Test {
 protected:
  void SetUp() {}

  void TearDown() {
    FftsPlusUpdateManager::Instance().creators_.clear();
  }
};

TEST_F(UtestFftsPlusSubTaskFactory, test_update_tegister_creator) {
  ASSERT_EQ(FftsPlusUpdateManager::Instance().GetUpdater("TEST_AIV"), nullptr);

  FftsPlusUpdateManager::FftsPlusUpdateRegistrar aiv_000("TEST_AIV", nullptr);
  ASSERT_EQ(FftsPlusUpdateManager::Instance().GetUpdater("TEST_AIV"), nullptr);

  FftsPlusUpdateManager::FftsPlusUpdateRegistrar aiv_001("TEST_AIV", [](){ return MakeShared<FFTSPlusTaskUpdate>(); });
  ASSERT_NE(FftsPlusUpdateManager::Instance().GetUpdater("TEST_AIV"), nullptr);

  FftsPlusUpdateManager::FftsPlusUpdateRegistrar aiv_002("TEST_AIV", [](){ return MakeShared<FFTSPlusTaskUpdate>(); });
  ASSERT_NE(FftsPlusUpdateManager::Instance().GetUpdater("TEST_AIV"), nullptr);
}

TEST_F(UtestFftsPlusSubTaskFactory, test_sub_task_tegister_creator) {
  ASSERT_EQ(FftsPlusSubTaskFactory::Instance().Create("TEST_AIV"), nullptr);

  FftsPlusSubTaskFactory::FftsPlusSubTaskRegistrar aiv_000("TEST_AIV", nullptr);
  ASSERT_EQ(FftsPlusSubTaskFactory::Instance().Create("TEST_AIV"), nullptr);

  FftsPlusSubTaskFactory::FftsPlusSubTaskRegistrar aiv_001("TEST_AIV", [](){ return MakeShared<FftsPlusAiCoreTask>(); });
  ASSERT_NE(FftsPlusSubTaskFactory::Instance().Create("TEST_AIV"), nullptr);

  FftsPlusSubTaskFactory::FftsPlusSubTaskRegistrar aiv_002("TEST_AIV", [](){ return MakeShared<FftsPlusAiCoreTask>(); });
  ASSERT_NE(FftsPlusSubTaskFactory::Instance().Create("TEST_AIV"), nullptr);
}
} // namespace ge
