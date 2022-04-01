// Copyright (c) 2021 Huawei Technologies Co., Ltd
// All rights reserved.
//
// Licensed under the BSD 3-Clause License  (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#define private public
#define protected public
#include "common/plugin/ge_util.h"
#include "common/plugin/op_tiling_manager.h"
#include "common/plugin/plugin_manager.h"
#include "common/plugin/tbe_plugin_manager.h"

using namespace testing;
using namespace std;

namespace ge {
namespace {
const char *const kEnvName = "ASCEND_OPP_PATH";
}
class UtestPluginManager : public testing::Test {
 public:
  OpTilingManager tiling_manager;

 protected:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtestPluginManager, op_tiling_manager_clear_handles) {
  auto test_so_name = "test_fail.so";
  tiling_manager.handles_[test_so_name] = nullptr;
  tiling_manager.ClearHandles();
  ASSERT_EQ(tiling_manager.handles_.size(), 0);
  string env_value = "test";
  setenv(kEnvName, env_value.c_str(), 0);
  const char *env = getenv(kEnvName);
  ASSERT_NE(env, nullptr);
  auto path = tiling_manager.GetPath();
  ASSERT_EQ(path.size(), 0);
}

TEST_F(UtestPluginManager, test_plugin_manager_load) {
  PluginManager manager;
  auto test_so_name = "test_fail.so";
  manager.handles_[test_so_name] = nullptr;
  manager.ClearHandles_();
  ASSERT_EQ(manager.handles_.size(), 0);
  EXPECT_EQ(manager.LoadSo("./", {}), SUCCESS);
  int64_t file_size = 1;
  EXPECT_EQ(manager.ValidateSo("./", 1, file_size), SUCCESS);

  std::string path = GetModelPath();
  std::vector<std::string> pathList;
  pathList.push_back(path);

  std::vector<std::string> funcChkList;
  const std::string file_name = "libcce.so";
  funcChkList.push_back(file_name);

  system(("touch " + path + file_name).c_str());

  EXPECT_EQ(manager.Load("", pathList), SUCCESS);
  EXPECT_EQ(manager.Load(path, {}), SUCCESS);
  EXPECT_EQ(manager.LoadSo(path, {}), SUCCESS);

  path += file_name + "/";
  pathList.push_back(path);
  EXPECT_EQ(manager.LoadSo(path, pathList), SUCCESS);

  unlink(path.c_str());
}

TEST_F(UtestPluginManager, test_tbe_plugin_manager) {
  TBEPluginManager manager;
  auto test_so_name = "test_fail.so";
  manager.handles_vec_ = {nullptr};
  manager.ClearHandles_();
  string env_value = "test";
  setenv(kEnvName, env_value.c_str(), 0);
  const char *env = getenv(kEnvName);
  ASSERT_NE(env, nullptr);
  auto path = tiling_manager.GetPath();
  ASSERT_EQ(path.size(), 0);
}
}  // namespace ge