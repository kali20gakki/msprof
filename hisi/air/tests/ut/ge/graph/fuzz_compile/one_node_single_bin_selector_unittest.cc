/**
 * Copyright 2022 Huawei Technologies Co., Ltd
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

#include "graph/bin_cache/node_bin_selector_factory.h"
#include "hybrid/common/bin_cache/one_node_single_bin_selector.h"

namespace ge {
class UtestOneNodeSingleBinSelector : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestOneNodeSingleBinSelector, do_select_bin_succces) {
  auto selector = NodeBinSelectorFactory::GetInstance().GetNodeBinSelector(kOneNodeSingleBinMode);
  std::vector<domi::TaskDef> task_defs;
  EXPECT_NE(selector->SelectBin(nullptr, nullptr, task_defs), nullptr);
  EXPECT_EQ(selector->SelectBin(nullptr, nullptr, task_defs)->GetCacheItemId(), 0);
}
}  // namespace ge