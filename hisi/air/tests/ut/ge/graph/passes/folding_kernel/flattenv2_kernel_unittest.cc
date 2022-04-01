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
#include "host_kernels/flattenv2_kernel.h"

#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/fp16_t.h"
#include "common/op/ge_op_utils.h"
#include "common/types.h" 
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

class UtestGraphPassesFoldingKernelFlattenV2Kernel : public testing::Test {
 protected:
  void SetUp() {
    std::cout << "FlattenV2 kernel test begin." << std::endl;
  }
  void TearDown() {
    std::cout << "FlattenV2 kernel test end." << std::endl;
  }
};

TEST_F(UtestGraphPassesFoldingKernelFlattenV2Kernel, ValidInputTest) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>("FlattenV2", FLATTENV2);
  GeTensorDesc tensor_desc_x(GeShape({1, 2, 3}), FORMAT_NCHW, DT_INT32);
  op_desc->AddInputDesc("x",tensor_desc_x);
  op_desc->SetAttr("axis", AnyValue::CreateFrom<int64_t>(1));
  op_desc->SetAttr("end_axis", AnyValue::CreateFrom<int64_t>(-1));
  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  op_desc->AddOutputDesc(tensor_desc_out);
  NodePtr _node = graph->AddNode(op_desc);
  shared_ptr<Kernel> kernel = KernelFactory::Instance().Create(FLATTENV2);
  std::vector<uint8_t> data = {2,2,2,2,2,2};
  GeTensorPtr data_tensor = std::make_shared<GeTensor>(tensor_desc_x, data);
  std::vector<ConstGeTensorPtr> input = {data_tensor};
  std::vector<GeTensorPtr> outputs;
  Status verify_status = kernel->Compute(_node);
  ASSERT_EQ(ge::SUCCESS, verify_status);

  Status status = kernel->Compute(_node->GetOpDesc(), input, outputs);
  ASSERT_EQ(ge::SUCCESS, status);

  std::vector<int64_t> expect_shape = {1,6};
  std::vector<int64_t> actual_shape = outputs[0]->GetTensorDesc().GetShape().GetDims();
  ASSERT_EQ(expect_shape, actual_shape);
  std::vector<uint8_t> &expect_data = data;
  const uint8_t *actual_data = outputs[0]->GetData().GetData();
  for (size_t i = 0; i < expect_data.size(); i++) {
    ASSERT_EQ(*(actual_data+i), expect_data[i]);
  }
}

TEST_F(UtestGraphPassesFoldingKernelFlattenV2Kernel, InvalidAttrTest) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");
  ge::OpDescPtr op_desc = std::make_shared<ge::OpDesc>("FlattenV2", FLATTENV2);
  GeTensorDesc tensor_desc_x(GeShape({1, 2, 3}), FORMAT_NCHW, DT_INT32);
  op_desc->AddInputDesc("x", tensor_desc_x);
  op_desc->SetAttr("axis", AnyValue::CreateFrom<int64_t>(1));
  // end_axis is invalid
  op_desc->SetAttr("end_axis", AnyValue::CreateFrom<int64_t>(-5));
  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  op_desc->AddOutputDesc(tensor_desc_out);
  NodePtr _node = graph->AddNode(op_desc);
  shared_ptr<Kernel> kernel = KernelFactory::Instance().Create(FLATTENV2);
  std::vector<uint8_t> data = {2,2,2,2,2,2};
  GeTensorPtr data_tensor = std::make_shared<GeTensor>(tensor_desc_x, data);
  std::vector<ConstGeTensorPtr> input = {data_tensor};
  std::vector<GeTensorPtr> outputs;

  Status verify_status = kernel->Compute(_node);
  EXPECT_EQ(NOT_CHANGED, verify_status);

  Status status = kernel->Compute(_node->GetOpDesc(), input, outputs);
  EXPECT_EQ(NOT_CHANGED, status);
}