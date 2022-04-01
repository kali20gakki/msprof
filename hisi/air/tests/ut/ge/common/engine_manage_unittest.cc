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

#define private public
#define protected public
#include "plugin/engine/engine_manage.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class UtestEngineManage : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestEngineManage, Normal) {
    auto p1 = std::make_shared<EngineManager>();
}

TEST_F(UtestEngineManage, Abnormal) {
    auto p1 = std::make_shared<EngineManager>();
    std::map<std::string, DNNEnginePtr> engines_map;
    GetDNNEngineObjs(engines_map);
    const std::string fake_engine = "fake_engine";
    const std::string ai_core = "AIcoreEngine";
    DNNEnginePtr fake_engine_ptr = nullptr;

    Status status;
    (void)EngineManager::GetEngine(fake_engine);
    DNNEnginePtr aicore_engine_ptr = EngineManager::GetEngine(ai_core);
    status = EngineManager::RegisterEngine(ai_core, aicore_engine_ptr);
    EXPECT_EQ(status, ge::FAILED);
    status = EngineManager::RegisterEngine(fake_engine, fake_engine_ptr);
    EXPECT_EQ(status, ge::FAILED);
}

} // namespace ge