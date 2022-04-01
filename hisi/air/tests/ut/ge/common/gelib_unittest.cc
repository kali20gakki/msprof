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
#include "init/gelib.h"
#include "framework/omg/ge_init.h"
#undef private
#undef protected

using namespace std;
using namespace testing;

namespace ge {

class UtestGeLib : public testing::Test {
 protected:
  void SetUp() {
  }
  void TearDown() {
  }
};

TEST_F(UtestGeLib, Normal) {
    auto p1 = std::make_shared<GELib>();
}

TEST_F(UtestGeLib, InnerInitialize) {
    auto p1 = std::make_shared<GELib>();
    std::map<std::string, std::string> options;
    p1->init_flag_ = true;
    EXPECT_EQ(p1->InnerInitialize(options), SUCCESS);
}

TEST_F(UtestGeLib, SystemInitialize) {
    auto p1 = std::make_shared<GELib>();
    std::map<std::string, std::string> options;
    options[OPTION_GRAPH_RUN_MODE] = "1";
    p1->is_train_mode_ = true;
    EXPECT_EQ(p1->SystemInitialize(options), SUCCESS);
}

TEST_F(UtestGeLib, SetDefaultPrecisionMode) {
    auto p1 = std::make_shared<GELib>();
    std::map<std::string, std::string> options;
    options[OPTION_GRAPH_RUN_MODE] = "2";
    p1->SetDefaultPrecisionMode(options);
}

TEST_F(UtestGeLib, SetRTSocVersion) {
    auto p1 = std::make_shared<GELib>();
    std::map<std::string, std::string> options;
    std::map<std::string, std::string> new_options;
    options[SOC_VERSION] = "version";
    EXPECT_EQ(p1->SetRTSocVersion(options, new_options), SUCCESS);
}

TEST_F(UtestGeLib, SetAiCoreNum) {
    auto p1 = std::make_shared<GELib>();
    std::map<std::string, std::string> options;
    options[AICORE_NUM] = "1";
    EXPECT_EQ(p1->SetAiCoreNum(options), SUCCESS);
}

TEST_F(UtestGeLib, InitOptions) {
    auto p1 = std::make_shared<GELib>();
    std::map<std::string, std::string> options;
    options[OPTION_EXEC_SESSION_ID] = "1";
    options[OPTION_EXEC_DEVICE_ID] = "1";
    options[OPTION_EXEC_JOB_ID] = "1";
    options[OPTION_EXEC_IS_USEHCOM] = "true";
    options[OPTION_EXEC_IS_USEHVD] = "true";
    options[OPTION_EXEC_DEPLOY_MODE] = "true";
    options[OPTION_EXEC_POD_NAME] = "pod";
    options[OPTION_EXEC_PROFILING_MODE] = "true";
    options[OPTION_EXEC_PROFILING_OPTIONS] = "profiling";
    options[OPTION_EXEC_RANK_ID] = "1";
    options[OPTION_EXEC_RANK_TABLE_FILE] = "rank";
    p1->InitOptions(options);
}

TEST_F(UtestGeLib, GeInitInitialize) {
    std::map<std::string, std::string> options;
    EXPECT_EQ(GEInit::Initialize(options), SUCCESS);
}

TEST_F(UtestGeLib, GeInitFinalize) {
    EXPECT_EQ(GEInit::Finalize(), SUCCESS);
}

TEST_F(UtestGeLib, GeInitGetPath) {
    EXPECT_NE(GEInit::GetPath().size(), 0);
}

TEST_F(UtestGeLib, Finalize) {
	auto p1 = std::make_shared<GELib>();
	p1->init_flag_ = false;
    EXPECT_EQ(p1->Finalize(), SUCCESS);
    p1->init_flag_ = true;
    p1->is_train_mode_ = true;
    EXPECT_EQ(p1->Finalize(), SUCCESS);
}


} // namespace ge