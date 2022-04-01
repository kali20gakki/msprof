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

#include <list>

#define private public
#define protected public
#include "inc/memory_slice.h"
#undef private
#undef protected

using namespace std;
using namespace ffts;
using namespace ge;

class MemorySliceUTest : public testing::Test {
 protected:
	void SetUp() {
	}

	void TearDown() {
	}
};

TEST_F(MemorySliceUTest, GenerateDataCtxParam_failed) {
  ge::GeTensorDescPtr tensor_desc = std::make_shared<ge::GeTensorDesc>();
  vector<int64_t> dim = {1, 1};
  GeShape shape(dim);
  tensor_desc->SetShape(shape);
  tensor_desc->SetFormat(FORMAT_NC1HWC0);
  tensor_desc->SetDataType(DT_FLOAT);
  std::vector<DimRange> slice;
  ge::DataType dtype;
  int64_t burst_len;
  std::vector<DataContextParam> data_ctx;
  Status ret = MemorySlice::GenerateDataCtxParam(shape.GetDims(), slice, DT_FLOAT, burst_len, data_ctx);
  EXPECT_EQ(ffts::FAILED, ret);
}

TEST_F(MemorySliceUTest, GenerateDataCtxParam1) {
  ge::GeTensorDescPtr tensor_desc = std::make_shared<ge::GeTensorDesc>();
  vector<int64_t> dim1 = {4, 4, 4};
  GeShape shape(dim1);
  tensor_desc->SetShape(shape);
  tensor_desc->SetFormat(FORMAT_NC1HWC0);
  tensor_desc->SetDataType(DT_FLOAT);
  std::vector<DimRange> slice;
  DimRange dim;
  dim.higher = 4;
  dim.lower = 3;
  slice.emplace_back(dim);
  dim.higher = 4;
  dim.lower = 0;
  slice.emplace_back(dim);
  slice.emplace_back(dim);
  ge::DataType dtype;
  int64_t burst_len = 0xfffffff;
  std::vector<DataContextParam> data_ctx;
  Status ret = MemorySlice::GenerateDataCtxParam(shape.GetDims(), slice, DT_FLOAT, burst_len, data_ctx);
  EXPECT_EQ(SUCCESS, ret);
}