/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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
#include <iostream>
#include <string>
#include <vector>

#define protected public
#define private public
#include "graph_optimizer/fusion_common/fusion_pass_manager.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class FUSION_PASS_MANAGER_STEST: public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(FUSION_PASS_MANAGER_STEST, initialize_suc) {
  FusionPassManager fusion_pass_manager;
  fusion_pass_manager.init_flag_ = true;
  Status ret = fusion_pass_manager.Initialize("AIcoreEngine");
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FUSION_PASS_MANAGER_STEST, finalize_suc1) {
  FusionPassManager fusion_pass_manager;
  fusion_pass_manager.init_flag_ = false;
  Status ret = fusion_pass_manager.Finalize();
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FUSION_PASS_MANAGER_STEST, finalize_suc2) {
  FusionPassManager fusion_pass_manager;
  Status ret = fusion_pass_manager.Finalize();
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(FUSION_PASS_MANAGER_STEST, CloseFusionPassPlugin_suc) {
  FusionPassManager fusion_pass_manager;
  PluginManagerPtr plugin_manager_ptr = std::make_shared<PluginManager>("pass_file");
  fusion_pass_manager.fusion_pass_plugin_manager_vec_ = {plugin_manager_ptr};
  fusion_pass_manager.CloseFusionPassPlugin();
  size_t size_vec = fusion_pass_manager.fusion_pass_plugin_manager_vec_.size();
  EXPECT_EQ(0, size_vec);
}

TEST_F(FUSION_PASS_MANAGER_STEST, OpenPassPath_failed) {
  FusionPassManager fusion_pass_manager;
  string file_path;
  vector<string> get_pass_files;
  Status ret = fusion_pass_manager.OpenPassPath(file_path, get_pass_files);
  EXPECT_EQ(fe::FAILED, ret);
}