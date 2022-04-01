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
#include "host_kernels/kernel_utils.h"

#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/fp16_t.h"
#include "common/ge_inner_error_codes.h"
#include "common/op/attr_value_util.h"
#include "common/types.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/passes/dimension_compute_pass.h"
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

class UtestGraphPassesFoldingKernelkernelUtils : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

TEST_F(UtestGraphPassesFoldingKernelkernelUtils, KernelUtilsFailed) {
  int64_t d0 = static_cast<int64_t>(INT_MAX);
  int64_t d1 = d0 + 1;
  vector<int64_t> data = {d0, d1};

  vector<int64_t> dims_vec_0;
  vector<int32_t> data_vec_0 = {0};
  GeTensorDesc tensor_desc_0(GeShape(dims_vec_0), FORMAT_NCHW, DT_INT32);
  ConstGeTensorPtr tensor_0 =
      std::make_shared<GeTensor>(tensor_desc_0, (uint8_t *)data_vec_0.data(), data_vec_0.size() * sizeof(int32_t));

  vector<GeTensorPtr> outputs;

  shared_ptr<Kernel> kernel = KernelFactory::Instance().Create(PACK);
  Status status = KernelUtils::ConstructTensorDescWithData(tensor_desc_0, data, outputs, true);
  EXPECT_EQ(PARAM_INVALID, status);

  NodePtr node_ptr = nullptr;
  status = KernelUtils::CheckDimensionNodeInfo(node_ptr);
  EXPECT_EQ(FAILED, status);
  bool ret = KernelUtils::CheckFormatSupported(node_ptr);
  EXPECT_EQ(false, ret);

  ConstGeTensorPtr const_weight_ptr = nullptr;
  OpDescPtr op_desc_ptr = nullptr;
  ret = KernelUtils::CheckSizeForTransOp(const_weight_ptr, op_desc_ptr);
  EXPECT_EQ(false, ret);
}
