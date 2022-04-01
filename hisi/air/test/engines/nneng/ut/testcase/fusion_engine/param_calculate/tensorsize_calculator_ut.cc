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
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"
#define protected public
#define private public
#include "param_calculate/tensorsize_calculator.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class TENSORSIZE_CALCULATOR_UTEST: public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(TENSORSIZE_CALCULATOR_UTEST, CalcSingleTensorSize_suc) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim = {-1, 1, 1, 1};
  GeShape shape(dim);
  GeTensorDesc in_desc(shape);
  in_desc.SetDataType(DT_FLOAT16);
  op_desc->AddInputDesc(in_desc);
  NodePtr node_0 = graph->AddNode(op_desc);
  ge::GeTensorDescPtr tensor_desc_ptr = op_desc->MutableInputDesc(0);
  string direction;
  size_t i = 0;
  bool output_real_calc_flag = false;
  int64_t tensor_size;

  TensorSizeCalculator tensor_size_calculator;
  Status ret = tensor_size_calculator.CalcSingleTensorSize(*op_desc, tensor_desc_ptr, direction, i,
                                                           output_real_calc_flag, tensor_size);
  EXPECT_EQ(ret, fe::SUCCESS);
}

TEST_F(TENSORSIZE_CALCULATOR_UTEST, CalcSingleTensorSize_failed) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim = {1, 1, 1, 1};
  GeShape shape(dim);
  GeTensorDesc in_desc(shape);
  in_desc.SetDataType(DT_MAX);
  op_desc->AddInputDesc(in_desc);
  NodePtr node_0 = graph->AddNode(op_desc);
  ge::GeTensorDescPtr tensor_desc_ptr = op_desc->MutableInputDesc(0);
  string direction;
  size_t i = 0;
  bool output_real_calc_flag = false;
  int64_t tensor_size;

  TensorSizeCalculator tensor_size_calculator;
  Status ret = tensor_size_calculator.CalcSingleTensorSize(*op_desc, tensor_desc_ptr, direction, i,
                                                           output_real_calc_flag, tensor_size);
  EXPECT_EQ(ret, fe::FAILED);
}

TEST_F(TENSORSIZE_CALCULATOR_UTEST, SpecialOutput) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim = {100, 200, 300, 1};
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetDataType(DT_FLOAT16);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  NodePtr node_0 = graph->AddNode(op_desc);
  ge::GeTensorDescPtr in_tensor = op_desc->MutableInputDesc(0);
  ge::GeTensorDescPtr out_tensor = op_desc->MutableOutputDesc(0);

  ge::AttrUtils::SetInt(in_tensor, ge::ATTR_NAME_SPECIAL_OUTPUT_SIZE, 5);
  ge::AttrUtils::SetInt(out_tensor, ge::ATTR_NAME_SPECIAL_OUTPUT_SIZE, 5);

  int output_real_calc_flag = 0;
  Status ret = TensorSizeCalculator::CalcInputOpTensorSize(*op_desc, output_real_calc_flag);
  ret = TensorSizeCalculator::CalcOutputOpTensorSize(*op_desc, output_real_calc_flag);
  EXPECT_EQ(ret, fe::SUCCESS);

  int64_t in_tensor_size = 0;
  int64_t out_tensor_size = 0;

  ge::TensorUtils::GetSize(*in_tensor, in_tensor_size);
  EXPECT_EQ(in_tensor_size, 12000032);


  ge::TensorUtils::GetSize(*out_tensor, out_tensor_size);
  EXPECT_EQ(out_tensor_size, 64);
}

TEST_F(TENSORSIZE_CALCULATOR_UTEST, SpecialOutput_02) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim = {100, 200, 300, 1};
  GeShape shape(dim);
  GeTensorDesc tensor_desc(shape);
  tensor_desc.SetDataType(DT_FLOAT16);
  op_desc->AddInputDesc(tensor_desc);
  op_desc->AddOutputDesc(tensor_desc);
  NodePtr node_0 = graph->AddNode(op_desc);
  ge::GeTensorDescPtr in_tensor = op_desc->MutableInputDesc(0);
  ge::GeTensorDescPtr out_tensor = op_desc->MutableOutputDesc(0);

  ge::AttrUtils::SetInt(in_tensor, ge::ATTR_NAME_SPECIAL_OUTPUT_SIZE, 5);
  ge::AttrUtils::SetInt(out_tensor, ge::ATTR_NAME_SPECIAL_OUTPUT_SIZE, 5);

  int output_real_calc_flag = 1;
  Status ret = TensorSizeCalculator::CalcInputOpTensorSize(*op_desc, output_real_calc_flag);
  ret = TensorSizeCalculator::CalcOutputOpTensorSize(*op_desc, output_real_calc_flag);
  EXPECT_EQ(ret, fe::SUCCESS);

  int64_t in_tensor_size = 0;
  int64_t out_tensor_size = 0;

  ge::TensorUtils::GetSize(*in_tensor, in_tensor_size);
  EXPECT_EQ(in_tensor_size, 12000000);


  ge::TensorUtils::GetSize(*out_tensor, out_tensor_size);
  EXPECT_EQ(out_tensor_size, 5);
}
