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

#define protected public
#define private public
#include "host_kernels/slice_d_kernel.h"

#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/fp16_t.h"
#include "common/ge_inner_error_codes.h"
#include "common/op/attr_value_util.h"
#include "common/op/ge_op_utils.h"
#include "common/types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/types.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "inc/kernel_factory.h"
#undef protected
#undef private

using namespace testing;
using namespace ge;
using ge::SUCCESS;

class UtestGraphPassesFoldingKernelSliceDKernel : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestGraphPassesFoldingKernelSliceDKernel, SliceDCheck1) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("SLICED", SLICED);
  AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_T, (int64_t)DT_INT32);

  std::string name1 = "name1";
  GeTensorDesc output_desc1;
  op_desc_ptr->AddOutputDesc(name1, output_desc1);

  std::string name2 = "name2";
  GeTensorDesc output_desc2;
  op_desc_ptr->AddOutputDesc(name2, output_desc2);

  vector<int64_t> dims_vec_0;
  vector<int32_t> data_vec_0 = {1};
  GeTensorDesc tensor_desc_0(GeShape(dims_vec_0), FORMAT_NCHW, DT_INT32);
  ConstGeTensorPtr tensor_0 =
      std::make_shared<GeTensor>(tensor_desc_0, (uint8_t *)data_vec_0.data(), data_vec_0.size() * sizeof(int32_t));

  vector<ConstGeTensorPtr> input = {tensor_0};
  vector<GeTensorPtr> outputs;

  SliceDKernel slice_d_kernel;
  std::vector<int64_t> begin_list;
  std::vector<int64_t> size_list;
  Status status = slice_d_kernel.SliceDCheck(op_desc_ptr, input, begin_list, size_list);
  EXPECT_EQ(PARAM_INVALID, status);
}

TEST_F(UtestGraphPassesFoldingKernelSliceDKernel, SliceDCheck2) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("SLICED", SLICED);
  AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_T, (int64_t)DT_INT32);

  std::string name1 = "name1";
  GeTensorDesc output_desc1;
  op_desc_ptr->AddOutputDesc(name1, output_desc1);

  std::string name2 = "name2";
  GeTensorDesc output_desc2;
  op_desc_ptr->AddOutputDesc(name2, output_desc2);

  GeTensorDesc input_desc1;
  op_desc_ptr->AddInputDesc(0, input_desc1);

  vector<int64_t> dims_vec_0;
  vector<int32_t> data_vec_0 = {1};
  GeTensorDesc tensor_desc_0(GeShape(dims_vec_0), FORMAT_NCHW, DT_INT32);
  ConstGeTensorPtr tensor_0 =
      std::make_shared<GeTensor>(tensor_desc_0, (uint8_t *)data_vec_0.data(), data_vec_0.size() * sizeof(int32_t));

  vector<ConstGeTensorPtr> input = {tensor_0};

  SliceDKernel slice_d_kernel;
  std::vector<int64_t> begin_list;
  std::vector<int64_t> size_list;
  Status status = slice_d_kernel.SliceDCheck(op_desc_ptr, input, begin_list, size_list);
  EXPECT_EQ(PARAM_INVALID, status);
}

TEST_F(UtestGraphPassesFoldingKernelSliceDKernel, SliceDCheck3) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("SLICED", SLICED);
  AttrUtils::SetInt(op_desc_ptr, ATTR_NAME_T, (int64_t)DT_INT32);

  std::string name1 = "name1";
  GeTensorDesc output_desc1;
  op_desc_ptr->AddOutputDesc(name1, output_desc1);

  GeTensorDesc input_desc1;
  op_desc_ptr->AddInputDesc(0, input_desc1);

  vector<int64_t> dims_vec_0;
  vector<int32_t> data_vec_0 = {1};
  GeTensorDesc tensor_desc_0(GeShape(dims_vec_0), FORMAT_NCHW, DT_INT32);
  ConstGeTensorPtr tensor_0 =
      std::make_shared<GeTensor>(tensor_desc_0, (uint8_t *)data_vec_0.data(), data_vec_0.size() * sizeof(int32_t));

  vector<ConstGeTensorPtr> input = {tensor_0};

  SliceDKernel slice_d_kernel;
  std::vector<int64_t> begin_list;
  std::vector<int64_t> size_list;
  Status status = slice_d_kernel.SliceDCheck(op_desc_ptr, input, begin_list, size_list);
  EXPECT_EQ(PARAM_INVALID, status);
}

TEST_F(UtestGraphPassesFoldingKernelSliceDKernel, ComputeParam1) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("SLICED", SLICED);
  GeTensorDesc dims_tensor_desc(GeShape({1, 1, 1, 1}), FORMAT_NCHW, DT_FLOAT);
  op_desc_ptr->AddOutputDesc(dims_tensor_desc);

  vector<ConstGeTensorPtr> input = {};
  vector<GeTensorPtr> outputs;

  SliceDKernel slice_d_kernel;
  Status status = slice_d_kernel.Compute(op_desc_ptr, input, outputs);
  EXPECT_NE(PARAM_INVALID, status);
}

TEST_F(UtestGraphPassesFoldingKernelSliceDKernel, ComputeParam2) {
  OpDescPtr op_desc_ptr = nullptr;

  int32_t start = 1, limit = 20, delta = 2;

  vector<int64_t> dims_vec_0;
  vector<int32_t> data_vec_0 = {start};
  GeTensorDesc tensor_desc_0(GeShape(dims_vec_0), FORMAT_NCHW, DT_INT32);
  ConstGeTensorPtr tensor_0 =
      std::make_shared<GeTensor>(tensor_desc_0, (uint8_t *)data_vec_0.data(), data_vec_0.size() * sizeof(int32_t));

  vector<int64_t> dims_vec_1;
  vector<int32_t> data_vec_1 = {limit};
  GeTensorDesc tensor_desc_1(GeShape(dims_vec_1), FORMAT_NCHW, DT_INT32);
  ConstGeTensorPtr tensor_1 =
      std::make_shared<GeTensor>(tensor_desc_1, (uint8_t *)data_vec_1.data(), data_vec_1.size() * sizeof(int32_t));

  vector<int64_t> dims_vec_2;
  vector<int32_t> data_vec_2 = {delta};
  GeTensorDesc tensor_desc_2(GeShape(dims_vec_2), FORMAT_NCHW, DT_INT32);
  ConstGeTensorPtr tensor_2 =
      std::make_shared<GeTensor>(tensor_desc_2, (uint8_t *)data_vec_2.data(), data_vec_2.size() * sizeof(int32_t));

  vector<ConstGeTensorPtr> input = {tensor_0, tensor_1, tensor_2};
  vector<GeTensorPtr> outputs;

  SliceDKernel slice_d_kernel;
  Status status = slice_d_kernel.Compute(op_desc_ptr, input, outputs);
  EXPECT_NE(SUCCESS, status);
}

TEST_F(UtestGraphPassesFoldingKernelSliceDKernel, CheckOutputDims1) {
  OpDescPtr op_desc_ptr = nullptr;
  std::vector<int64_t> output_dims = {1,1,1};

  SliceDKernel slice_d_kernel;
  Status status = slice_d_kernel.CheckOutputDims(output_dims, op_desc_ptr);
  EXPECT_EQ(SUCCESS, status);
}
