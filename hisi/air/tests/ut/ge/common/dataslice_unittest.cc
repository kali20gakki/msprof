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

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include "slice/data_slice_helper.h"
#include "slice/data_slice_toolkit.h"
#include "slice/data_slice_factory.h"
#include "inc/register/infer_axis_slice_registry.h"
#include "framework/common/debug/ge_log.h"
#include "graph/operator_factory_impl.h"
#include "framework/common/util.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/type_utils.h"

using namespace std;
using namespace testing;

namespace ge {
class DataSlice : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};
IMPLEMT_COMMON_INFER_AXIS_TYPE_INFO(Temp) {
  axis_type = {};
  return GRAPH_SUCCESS;
}
INFER_AXIS_TYPE_INFO_REG(Add, Temp);
INFER_AXIS_TYPE_INFO_REG(Cast, Temp);
IMPLEMT_COMMON_INFER_AXIS_SLICE(Temp1) {
  input_param = {{{0,0},{},{},{}}};
  return GRAPH_SUCCESS;
}
INFER_AXIS_SLICE_FUNC_REG(Add, Temp1);
TEST_F(DataSlice, data_slice_helper_1) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("Add", "Add");
  GeTensorDesc output_desc;
  op_desc->AddOutputDesc("output", output_desc);
  GeTensorDesc input_desc;
  op_desc->AddInputDesc("input", input_desc);
  AxisTypeInfo axis_type_info;
  axis_type_info.SetAxisType(AxisType::ELEMENTWISE);
  std::pair<int64_t, std::vector<int64_t>> input_cut_info(0, {0});
  axis_type_info.AddInputCutInfo(input_cut_info);
  std::pair<int64_t, std::vector<int64_t>> output_cut_info(0, {0});
  axis_type_info.AddOutputCutInfo(output_cut_info);
  Status ret = DataSliceHelper::InferAxisSlice(op_desc, axis_type_info);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(DataSlice, data_slice_helper_2) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("Cast", "Cast");
  GeTensorDesc output_desc;
  op_desc->AddOutputDesc("output", output_desc);
  GeTensorDesc input_desc;
  op_desc->AddInputDesc("input", input_desc);
  AxisTypeInfo axis_type_info;
  axis_type_info.SetAxisType(AxisType::ELEMENTWISE);
  std::pair<int64_t, std::vector<int64_t>> input_cut_info(0, {0});
  axis_type_info.AddInputCutInfo(input_cut_info);
  std::pair<int64_t, std::vector<int64_t>> output_cut_info(0, {0});
  axis_type_info.AddOutputCutInfo(output_cut_info);
  Status ret = DataSliceHelper::InferAxisSlice(op_desc, axis_type_info);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(DataSlice, data_slice_helper_3) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("Cast", "Cast");
  GeTensorDesc output_desc;
  op_desc->AddOutputDesc("output", output_desc);
  GeTensorDesc input_desc;
  op_desc->AddInputDesc("input", input_desc);
  AxisTypeInfo axis_type_info;
  axis_type_info.SetAxisType(AxisType::UNSPLIT);
  std::pair<int64_t, std::vector<int64_t>> input_cut_info(0, {0});
  axis_type_info.AddInputCutInfo(input_cut_info);
  std::pair<int64_t, std::vector<int64_t>> output_cut_info(0, {0});
  axis_type_info.AddOutputCutInfo(output_cut_info);
  Status ret = DataSliceHelper::InferAxisSlice(op_desc, axis_type_info);
  EXPECT_EQ(ret, SUCCESS);
}

TEST_F(DataSlice, data_slice_helper_4) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("Cast", "Cast");
  GeTensorDesc output_desc;
  op_desc->AddOutputDesc("output", output_desc);
  GeTensorDesc input_desc;
  op_desc->AddInputDesc("input", input_desc);
  AxisTypeInfo axis_type_info;
  axis_type_info.SetAxisType(AxisType::SLIDINGWINDOW);
  std::pair<int64_t, std::vector<int64_t>> input_cut_info(0, {0});
  axis_type_info.AddInputCutInfo(input_cut_info);
  std::pair<int64_t, std::vector<int64_t>> output_cut_info(0, {0});
  axis_type_info.AddOutputCutInfo(output_cut_info);
  Status ret = DataSliceHelper::InferAxisSlice(op_desc, axis_type_info);
  EXPECT_EQ(ret, FAILED);
}

TEST_F(DataSlice, data_slice_helper_5) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("Cast", "Cast");
  GeTensorDesc output_desc;
  op_desc->AddOutputDesc("output", output_desc);
  GeTensorDesc input_desc;
  op_desc->AddInputDesc("input", input_desc);
  std::vector<AxisTypeInfo> axis_type_info;
  Status ret = DataSliceHelper::GetSliceInfo(op_desc, axis_type_info);
  EXPECT_EQ(ret, SUCCESS);
}
}
