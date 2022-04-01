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
#include <nlohmann/json.hpp>
#include "fcntl.h"

#define protected public
#define private public
#include <graph/tensor.h>
#include "graph_optimizer/ffts/tbe_sgt_json_parse.h"
#include "graph_optimizer/ffts/tbe_json_parse_impl.h"
#include "common/fe_utils.h"
#include "common/fe_log.h"
#include "graph/ge_tensor.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;
using namespace nlohmann;

class STEST_FE_TBE_SGT_JSON_PARSER: public testing::Test
{
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_json_auto_thread_mix_aic_aiv)
{
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  vector<string> json_file_path =
          {"./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_9a43f1_non_tail.json",
           "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_9a43f1_tail.json"};
  vector<string> bin_file_path;
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  json_file_parse.ParseTvmWorkSpace();
  EXPECT_EQ(ret, fe::FAILED);
}


TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_ParseTvm_1_failed) { 
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  vector<string> json_file_path =
          {"./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_failed_1.json",
           "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_failed_1.json"};
  vector<string> bin_file_path;
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  ret = json_file_parse.ParseTvmBlockDim();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseBatchBindOnly();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmMagic();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmCoreType();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmModeInArgsFirstField();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmWorkSpace();
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_ParseTvm_0_success) { 
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  vector<string> json_file_path =
          {"./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_suc.json",
           "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_suc.json"};
  vector<string> bin_file_path;
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  ret = json_file_parse.ParseTvmTaskRatio();
  EXPECT_EQ(fe::SUCCESS, ret);
  ret = json_file_parse.ParseConvCompressParameters();
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_GetWorkspaceAtomicFlagAndOutputIndexFlag) { 
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  std::vector<int64_t> parameters_index = {1, 2, 3};
  size_t workspace_num = 1;
  size_t input_num = 1;
  size_t output_num = 1;
  std::vector<int64_t> cur_output_index;
  int64_t workspace_atomic_flag;
  bool output_index_flag = false;
  json_file_parse.GetWorkspaceAtomicFlagAndOutputIndexFlag(parameters_index, workspace_num, input_num, output_num,
                                                           cur_output_index, workspace_atomic_flag, output_index_flag);
  EXPECT_EQ(true, output_index_flag);
}

TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_PackageTvmJsonInfo) { 
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ffts::ThreadSliceMap thread_slice_map;
  thread_slice_map.slice_instance_num = 2;
  ffts::ThreadSliceMapPtr thread_slice_map_ptr = std::make_shared<ffts::ThreadSliceMap>(thread_slice_map);
  op_desc_ptr->SetExtAttr(ffts::kAttrSgtStructInfo, thread_slice_map_ptr);
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  vector<string> json_file_path =
          {"./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_9a43f1_non_tail_mix_aic.json",
           "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_9a43f1_non_tail_mix_aic.json"};
  vector<string> bin_file_path;
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  ret = json_file_parse.ParseTvmWorkSpace();
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_parse_tvm_faield) { 
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  vector<string> json_file_path =
          {"./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sigmoid_failed.json"};
  vector<string> bin_file_path;
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  ret = json_file_parse.ParseTvmBlockDim();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseBatchBindOnly();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmMagic();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmTaskRatio();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmModeInArgsFirstField();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmWorkSpace();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseTvmParameters();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseConvCompressParameters();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseWeightRepeat();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseOpParaSize();
  EXPECT_EQ(fe::FAILED, ret);
  ret = json_file_parse.ParseOpKBHitrate();
  EXPECT_EQ(fe::FAILED, ret);
}

TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_Parse_magic_success) {
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  vector<string> json_file_path =
      {"./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sgt_success_1.json",
       "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sgt_success_2.json"};
  vector<string> bin_file_path;
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  ret = json_file_parse.ParseTvmMagic();
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(STEST_FE_TBE_SGT_JSON_PARSER, case_parse_kbhit_failed) {
  OpDescPtr op_desc_ptr = make_shared<ge::OpDesc>("Sigmoid", "sigmoid");
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  NodePtr node = graph->AddNode(op_desc_ptr);
  TbeSgtJsonFileParse json_file_parse(*node);
  vector<string> json_file_path =
      {"./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sgt_success_1.json",
       "./air/test/engines/nneng/ut/testcase/fusion_engine/ffts/json/te_sgt_success_2.json"};
  vector<string> bin_file_path;
  Status ret = json_file_parse.PackageTvmJsonInfo(json_file_path, bin_file_path);
  ret = json_file_parse.ParseOpKBHitrate();
  EXPECT_EQ(fe::FAILED, ret);
}