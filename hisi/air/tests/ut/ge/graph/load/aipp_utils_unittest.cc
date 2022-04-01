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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#define private public
#define protected public
#include "ut/ge/ffts_plus_proto_tools.h"
#include "graph/utils/graph_utils.h"
#include "graph/load/model_manager/aipp_utils.h"

using namespace std;
using namespace testing;

namespace ge {

class UtestAippUtils : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};


TEST_F(UtestAippUtils, test_AippUtils_SetMatrixInfo) {
  domi::AippOpParams aipp_params;
  aipp_params.add_matrix_r0c0(32);
  aipp_params.add_matrix_r0c1(32);
  aipp_params.add_matrix_r0c2(32);
  aipp_params.add_matrix_r1c0(32);
  aipp_params.add_matrix_r1c1(32);
  aipp_params.add_matrix_r1c2(32);
  aipp_params.add_matrix_r2c0(32);
  aipp_params.add_matrix_r2c1(32);
  aipp_params.add_matrix_r2c2(32);
  AippConfigInfo aipp_info;
  AippUtils::SetMatrixInfo(aipp_params, aipp_info);

  EXPECT_EQ(aipp_info.matrix_r0c0, 32);
  EXPECT_EQ(aipp_info.matrix_r0c1, 32);
  EXPECT_EQ(aipp_info.matrix_r0c2, 32);
  EXPECT_EQ(aipp_info.matrix_r1c0, 32);
  EXPECT_EQ(aipp_info.matrix_r1c1, 32);
  EXPECT_EQ(aipp_info.matrix_r1c2, 32);
  EXPECT_EQ(aipp_info.matrix_r2c0, 32);
  EXPECT_EQ(aipp_info.matrix_r2c1, 32);
  EXPECT_EQ(aipp_info.matrix_r2c2, 32);
}

TEST_F(UtestAippUtils, test_AippUtils_SetBiasInfo) {
  domi::AippOpParams aipp_params;
  aipp_params.add_output_bias_0(32);
  aipp_params.add_output_bias_1(32);
  aipp_params.add_output_bias_2(32);
  aipp_params.add_input_bias_0(32);
  aipp_params.add_input_bias_1(32);
  aipp_params.add_input_bias_2(32);
  AippConfigInfo aipp_info;
  AippUtils::SetBiasInfo(aipp_params, aipp_info);

  EXPECT_EQ(aipp_info.output_bias_0, 32);
  EXPECT_EQ(aipp_info.output_bias_1, 32);
  EXPECT_EQ(aipp_info.output_bias_2, 32);
  EXPECT_EQ(aipp_info.input_bias_0, 32);
  EXPECT_EQ(aipp_info.input_bias_1, 32);
  EXPECT_EQ(aipp_info.input_bias_2, 32);
}

TEST_F(UtestAippUtils, test_AippUtils_SetChnInfo) {
  domi::AippOpParams aipp_params;
  aipp_params.add_var_reci_chn_0(32);
  aipp_params.add_var_reci_chn_1(32);
  aipp_params.add_var_reci_chn_2(32);
  aipp_params.add_var_reci_chn_3(32);
  AippConfigInfo aipp_info;
  AippUtils::SetChnInfo(aipp_params, aipp_info);

  EXPECT_EQ(aipp_info.var_reci_chn_0, 32);
  EXPECT_EQ(aipp_info.var_reci_chn_1, 32);
  EXPECT_EQ(aipp_info.var_reci_chn_2, 32);
  EXPECT_EQ(aipp_info.var_reci_chn_3, 32);
}






}