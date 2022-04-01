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
#include "plugin/engine/dnnengines.h"

#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class UtestDnnengines : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestDnnengines, Normal) {
    auto p1 = std::make_shared<AICoreDNNEngine>("name");
    auto p2 = std::make_shared<VectorCoreDNNEngine>("name");
    auto p3 = std::make_shared<AICpuDNNEngine>("name");
    auto p4 = std::make_shared<AICpuTFDNNEngine>("name");
    auto p5 = std::make_shared<GeLocalDNNEngine>("name");
    auto p6 = std::make_shared<HostCpuDNNEngine>("name");
    auto p7 = std::make_shared<RtsDNNEngine>("name");
    auto p8 = std::make_shared<HcclDNNEngine>("name");
    auto p9 = std::make_shared<FftsPlusDNNEngine>("name");
    auto p10 = std::make_shared<DSADNNEngine>("name");
}



} // namespace ge
