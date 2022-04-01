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
#include "common/fusion_statistic/buffer_fusion_info_collecter.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class BUFFER_FUSION_INFO_COLLECTOR_STEST: public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(BUFFER_FUSION_INFO_COLLECTOR_STEST, get_pass_name_of_scope_id_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  ge::AttrUtils::SetInt(op_desc_0, SCOPE_ID_ATTR, 1);
  ge::AttrUtils::SetStr(op_desc_0, "pass_name", "add_pass_name");
  NodePtr node_0 = graph->AddNode(op_desc_0);
  PassNameIdMap pass_name_scope_id_map;

  BufferFusionInfoCollecter buffer_fusion_info_collecter;
  Status ret = buffer_fusion_info_collecter.GetPassNameOfScopeId(*graph, pass_name_scope_id_map);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(BUFFER_FUSION_INFO_COLLECTOR_STEST, get_pass_name_of_failed_id_success) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  ge::AttrUtils::SetInt(op_desc_0, "fusion_failed", 1);
  ge::AttrUtils::SetStr(op_desc_0, "pass_name", "add_pass_name");
  NodePtr node_0 = graph->AddNode(op_desc_0);
  PassNameIdMap pass_name_fusion_failed_id_map;

  BufferFusionInfoCollecter buffer_fusion_info_collecter;
  Status ret = buffer_fusion_info_collecter.GetPassNameOfFailedId(*graph, pass_name_fusion_failed_id_map);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(BUFFER_FUSION_INFO_COLLECTOR_STEST, count_buffer_fusion_times_test) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddOutputDesc(out_desc);
  ge::AttrUtils::SetInt(op_desc_0, SCOPE_ID_ATTR, 1);
  ge::AttrUtils::SetStr(op_desc_0, "pass_name", "add_pass_name");
  NodePtr node_0 = graph->AddNode(op_desc_0);
  std::set<std::string> pass_name_set = {"match", "effect"};
  BufferFusionInfoCollecter buffer_fusion_info_collecter;
  buffer_fusion_info_collecter.SetPassName(*graph, pass_name_set);
  PassNameIdMap pass_name_scope_id_map;
  Status ret = buffer_fusion_info_collecter.GetPassNameOfScopeId(*graph, pass_name_scope_id_map);
  EXPECT_EQ(ret, fe::SUCCESS);
  PassNameIdMap pass_name_fusion_failed_id_map;
  ret = buffer_fusion_info_collecter.GetPassNameOfFailedId(*graph, pass_name_fusion_failed_id_map);
  EXPECT_EQ(ret, fe::SUCCESS);
  std::map<std::string, int32_t> pass_match_map;
  pass_match_map["match"] = 1;
  std::map<std::string, int32_t> pass_effect_map;
  pass_match_map["effect"] = 1;
  ret = buffer_fusion_info_collecter.CountBufferFusionTimes(*graph);
  EXPECT_EQ(ret, fe::SUCCESS);
}