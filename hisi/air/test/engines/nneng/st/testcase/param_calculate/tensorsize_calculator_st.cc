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
#include "param_calculate/tensorsize_calculator.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class TENSORSIZE_CALCULATOR_STEST: public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(TENSORSIZE_CALCULATOR_STEST, CalcSingleTensorSize_suc) {
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

TEST_F(TENSORSIZE_CALCULATOR_STEST, CalcSingleTensorSize_failed) {
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