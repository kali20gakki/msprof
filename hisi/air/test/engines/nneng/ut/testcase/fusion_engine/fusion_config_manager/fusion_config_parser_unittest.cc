/**
 * Copyright 2019-2020 Huawei Technologies Co., Ltd
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

#define protected public
#define private public
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/compute_graph.h"
#include "common/configuration.h"
#include "common/util/op_info_util.h"
#include "fusion_config_manager/fusion_config_parser.h"
#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace fe;
using FusionConfigParserPtr = std::shared_ptr<FusionConfigParser>;
class UTestFusionConfigParser : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
  std::string  allStr = "ALL";
  std::string  nullStr = "";
};

TEST_F(UTestFusionConfigParser, fusion_switch_01) {
  Configuration::Instance(fe::AI_CORE_NAME).lib_path_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config/";
  Configuration::Instance(fe::AI_CORE_NAME).custom_fusion_config_file_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config.json";
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(allStr);
  FusionConfigParserPtr fusionConfigParserPtr = std::make_shared<FusionConfigParser>(fe::AI_CORE_NAME);
  fusionConfigParserPtr->ParseFusionConfigFile();
  bool ret = fusionConfigParserPtr->GetFusionSwitchByName("PassThroughFusionPass", "GraphFusion");
  EXPECT_EQ(ret, true);
  bool ret1 = fusionConfigParserPtr->GetFusionSwitchByName("BUILTIN_PASS1", "GraphFusion");
  EXPECT_EQ(ret1, true);
  bool ret2 = fusionConfigParserPtr->GetFusionSwitchByName("CUSTOM_PASS1", "GraphFusion");
  EXPECT_EQ(ret2, false);
  bool ret3 = fusionConfigParserPtr->GetFusionSwitchByName("CUSTOM_PASS2", "GraphFusion");
  EXPECT_EQ(ret3, false);
  bool ret4 = fusionConfigParserPtr->GetFusionSwitchByName("TbeCommonRules2FusionPass", "UBFusion");
  EXPECT_EQ(ret4, true);
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(nullStr);
}

TEST_F(UTestFusionConfigParser, fusion_switch_02) {
  Configuration::Instance(fe::AI_CORE_NAME).lib_path_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config1/";
  Configuration::Instance(fe::AI_CORE_NAME).custom_fusion_config_file_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config1.json";
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(allStr);

  FusionConfigParserPtr fusionConfigParserPtr = std::make_shared<FusionConfigParser>(fe::AI_CORE_NAME);
  fusionConfigParserPtr->ParseFusionConfigFile();
  bool ret = fusionConfigParserPtr->GetFusionSwitchByName("PassThroughFusionPass", "GraphFusion");
  EXPECT_EQ(ret, true);
  bool ret1 = fusionConfigParserPtr->GetFusionSwitchByName("BUILTIN_PASS1", "GraphFusion");
  EXPECT_EQ(ret1, true);
  ret1 = fusionConfigParserPtr->GetFusionSwitchByName("BUILTIN_PASS2", "GraphFusion");
  EXPECT_EQ(ret1, true);
  bool ret2 = fusionConfigParserPtr->GetFusionSwitchByName("CUSTOM_PASS1", "GraphFusion");
  EXPECT_EQ(ret2, false);
  bool ret3 = fusionConfigParserPtr->GetFusionSwitchByName("CUSTOM_PASS2", "GraphFusion");
  EXPECT_EQ(ret3, true);
  bool ret4 = fusionConfigParserPtr->GetFusionSwitchByName("TbeCommonRules2FusionPass", "UBFusion");
  EXPECT_EQ(ret4, true);
  ret4 = fusionConfigParserPtr->GetFusionSwitchByName("UB_FUSION_PASS1", "UBFusion");
  EXPECT_EQ(ret4, false);
  ret4 = fusionConfigParserPtr->GetFusionSwitchByName("UB_FUSION_PASS2", "UBFusion");
  EXPECT_EQ(ret4, true);
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(nullStr);
}

TEST_F(UTestFusionConfigParser, fusion_switch_03) {
  std::string  allStr = "ALL";
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(allStr);
  Configuration::Instance(fe::AI_CORE_NAME).lib_path_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config2/";
  Configuration::Instance(fe::AI_CORE_NAME).custom_fusion_config_file_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config2.json";
  FusionConfigParserPtr fusionConfigParserPtr = std::make_shared<FusionConfigParser>(fe::AI_CORE_NAME);
  fusionConfigParserPtr->ParseFusionConfigFile();
  bool ret = fusionConfigParserPtr->GetFusionSwitchByName("PassThroughFusionPass", "GraphFusion");
  EXPECT_EQ(ret, true);
  ret = fusionConfigParserPtr->GetFusionSwitchByName("BUILTIN_PASS1", "GraphFusion");
  EXPECT_EQ(ret, true);
  ret = fusionConfigParserPtr->GetFusionSwitchByName("CUSTOM_PASS1", "GraphFusion");
  EXPECT_EQ(ret, false);
  ret = fusionConfigParserPtr->GetFusionSwitchByName("CUSTOM_PASS2", "GraphFusion");
  EXPECT_EQ(ret, false);
  ret = fusionConfigParserPtr->GetFusionSwitchByName("TbeCommonRules2FusionPass", "UBFusion");
  EXPECT_EQ(ret, true);
  ret = fusionConfigParserPtr->GetFusionSwitchByName("UB_PASS1", "UBFusion");
  EXPECT_EQ(ret, false);
  ret = fusionConfigParserPtr->GetFusionSwitchByName("UB_PASS2", "UBFusion");
  EXPECT_EQ(ret, true);
  ret = fusionConfigParserPtr->GetFusionSwitchByName("UB_PASS3", "UBFusion");
  EXPECT_EQ(ret, false);
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(nullStr);
}

TEST_F(UTestFusionConfigParser, fusion_switch_04) {
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(allStr);
  Configuration::Instance(fe::AI_CORE_NAME).lib_path_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config2/";
  Configuration::Instance(fe::AI_CORE_NAME).custom_fusion_config_file_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config2.json";
  FusionConfigParserPtr fusionConfigParserPtr = std::make_shared<FusionConfigParser>(fe::AI_CORE_NAME);
  fusionConfigParserPtr->ParseFusionConfigFile();
  bool ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("PassThroughFusionPass", "GraphFusion", "ALL");
  EXPECT_EQ(ret, false);
  ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("BUILTIN_PASS1", "GraphFusion", "ALL");
  EXPECT_EQ(ret, true);
  ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("CUSTOM_PASS1", "GraphFusion", "ALL");
  EXPECT_EQ(ret, false);
  ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("CUSTOM_PASS2", "GraphFusion", "ALL");
  EXPECT_EQ(ret, false);
  ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("TbeCommonRules2FusionPass", "UBFusion", "ALL");
  EXPECT_EQ(ret, true);
  ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("UB_PASS1", "UBFusion", "ALL");
  EXPECT_EQ(ret, false);
  ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("UB_PASS2", "UBFusion", "ALL");
  EXPECT_EQ(ret, true);
  ret = fusionConfigParserPtr->GetFusionSwitchByNameLicense("UB_PASS3", "UBFusion", "ALL");
  EXPECT_EQ(ret, false);
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(nullStr);
}

TEST_F(UTestFusionConfigParser, modify_fusion_priority_map_test) {
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(allStr);
  Configuration::Instance(fe::AI_CORE_NAME).lib_path_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/builtin_config2/";
  Configuration::Instance(fe::AI_CORE_NAME).custom_fusion_config_file_ =
          "./air/test/engines/nneng/ut/testcase/fusion_engine/fusion_config_manager/custom_config/fusion_config2.json";
  nlohmann::json common_pass_switch_file_json = { {"attr", "node1.attr1"}, {"value",{"value",{"value", {"value","node1.attr1"}}}}, {"expr", "="}};
  FusionConfigParserPtr fusionConfigParserPtr = std::make_shared<FusionConfigParser>(fe::AI_CORE_NAME);
  map<string, int32_t> map_temp{std::make_pair("value", 1)};
  string type_string = "value";
  string fusion_type_temp = "value";
  Status ret = fusionConfigParserPtr->ModifyFusionPriorityMap(common_pass_switch_file_json, map_temp, type_string, fusion_type_temp);
  EXPECT_EQ(ret, fe::SUCCESS);
  Configuration::Instance(fe::AI_CORE_NAME).InitLicenseFusion(nullStr);
}