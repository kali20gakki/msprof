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
#include <vector>
#include <string>
#include <map>
#include "common/mem_grp/memory_group_manager.h"

#define protected public
#define private public

namespace ge {
class UtMemoryGroupManager : public testing::Test {
 public:
  UtMemoryGroupManager() {}
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(UtMemoryGroupManager, run_MemGrpCreate) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGrpCreate(group_name);
}

TEST_F(UtMemoryGroupManager, run_MemGrpCreateFailed) {
  std::string group_name = "DN_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGrpCreate(group_name);
}

TEST_F(UtMemoryGroupManager, run_MemGrpAddProc) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGrpAddProc(group_name, 0, true, true);
}

TEST_F(UtMemoryGroupManager, run_MemGrpAddProcError) {
  std::string group_name = "DM_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGrpAddProc(group_name, 900000, true, true);
}

TEST_F(UtMemoryGroupManager, run_MemGrpAddProcFailed) {
  std::string group_name = "DN_QS_GROUP";
  ge::MemoryGroupManager::GetInstance().MemGrpAddProc(group_name, 0, true, true);
}

TEST_F(UtMemoryGroupManager, run_Initialize) {
  ge::MemoryGroupManager::GetInstance().Initialize();
}
}