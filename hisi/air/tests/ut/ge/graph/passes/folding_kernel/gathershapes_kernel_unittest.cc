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
#include "host_kernels/gathershapes_kernel.h"

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

class UtestGraphPassesFoldingKernelGatherShapesKernel : public testing::Test {
 protected:
  void SetUp() {
    std::cout << "GatherShapes kernel test begin." << std::endl;
  }
  void TearDown() {
    std::cout << "GatherShapes kernel test end." << std::endl;
  }
};

TEST_F(UtestGraphPassesFoldingKernelGatherShapesKernel, ValidInputTest) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("GatherShapes", "GatherShapes");
  GeTensorDesc tensor_desc_x1(GeShape({1, 2, 3}), FORMAT_NHWC, DT_INT32);
  GeTensorDesc tensor_desc_x2(GeShape({3, 4, 5}), FORMAT_NHWC, DT_INT32);
  op_desc_ptr->AddInputDesc(0, tensor_desc_x1);
  op_desc_ptr->AddInputDesc(1, tensor_desc_x2);
  op_desc_ptr->SetAttr("axes",
                       AnyValue::CreateFrom(std::vector<std ::vector<int64_t>>{{0, 1}, {0, 2}, {1, 0}, {1, 2}}));
  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  op_desc_ptr->AddOutputDesc(tensor_desc_out);
  shared_ptr<Kernel> gathershapes_kernel = KernelFactory::Instance().Create(GATHERSHAPES);
  ge::ComputeGraphPtr graph_ = std::make_shared<ge::ComputeGraph>("default");
  NodePtr node_ = graph_->AddNode(op_desc_ptr);
  std::vector<GeTensorPtr> v_output;
  ge::Status status = gathershapes_kernel->Compute(node_, v_output);
  ASSERT_EQ(ge::SUCCESS, status);
  ASSERT_EQ(v_output.size(), 1);
  vector<int64_t> expect_output({2, 3, 3, 5});
  GeTensorPtr tensor_out = v_output[0];
  int32_t *data_buf = (int32_t *)tensor_out->GetData().data();
  for (size_t i = 0; i < expect_output.size(); i++) {
    EXPECT_EQ(*(data_buf + i), expect_output[i]);
  }
}

TEST_F(UtestGraphPassesFoldingKernelGatherShapesKernel, InputWithUnknownRankTest) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("GatherShapes", "GatherShapes");
  GeTensorDesc tensor_desc_x1(GeShape({1, 2, 3}), FORMAT_NHWC, DT_INT32);
  GeTensorDesc tensor_desc_x2(GeShape({-2}), FORMAT_NHWC, DT_INT32);
  op_desc_ptr->AddInputDesc(0, tensor_desc_x1);
  op_desc_ptr->AddInputDesc(1, tensor_desc_x2);
  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  op_desc_ptr->AddOutputDesc(tensor_desc_out);
  op_desc_ptr->SetAttr("axes",
                       AnyValue::CreateFrom(std::vector<std ::vector<int64_t>>{{0, 1}, {0, 2}, {1, 0}, {1, 2}}));
  shared_ptr<Kernel> gathershapes_kernel = KernelFactory::Instance().Create("GatherShapes");
  ge::ComputeGraphPtr graph_ = std::make_shared<ge::ComputeGraph>("default");
  NodePtr node_ = graph_->AddNode(op_desc_ptr);
  std::vector<GeTensorPtr> v_output;
  ge::Status status = gathershapes_kernel->Compute(node_, v_output);
  EXPECT_EQ(ge::NOT_CHANGED, status);
}

TEST_F(UtestGraphPassesFoldingKernelGatherShapesKernel, InputWithUnknownShapeTest1) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("GatherShapes", "GatherShapes");
  GeTensorDesc tensor_desc_x1(GeShape({1, 2, 3}), FORMAT_NHWC, DT_INT32);
  GeTensorDesc tensor_desc_x2(GeShape({2, 3, -1}), FORMAT_NHWC, DT_INT32);
  op_desc_ptr->AddInputDesc(0, tensor_desc_x1);
  op_desc_ptr->AddInputDesc(1, tensor_desc_x2);
  op_desc_ptr->SetAttr("axes",
                       AnyValue::CreateFrom(std::vector<std ::vector<int64_t>>{{0, 1}, {0, 2}, {1, 0}, {1, 2}}));
  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  op_desc_ptr->AddOutputDesc(tensor_desc_out);
  shared_ptr<Kernel> gathershapes_kernel = KernelFactory::Instance().Create("GatherShapes");
  ge::ComputeGraphPtr graph_ = std::make_shared<ge::ComputeGraph>("default");
  NodePtr node_ = graph_->AddNode(op_desc_ptr);
  std::vector<GeTensorPtr> v_output;
  ge::Status status = gathershapes_kernel->Compute(node_, v_output);
  EXPECT_EQ(ge::NOT_CHANGED, status);
}

TEST_F(UtestGraphPassesFoldingKernelGatherShapesKernel, InputWithUnknownShapeTest2) {
  OpDescPtr op_desc_ptr = std::make_shared<OpDesc>("GatherShapes", "GatherShapes");
  GeTensorDesc tensor_desc_x1(GeShape({1, 2, 3}), FORMAT_NHWC, DT_INT32);
  GeTensorDesc tensor_desc_x2(GeShape({2, 3, -1}), FORMAT_NHWC, DT_INT32);
  op_desc_ptr->AddInputDesc(0, tensor_desc_x1);
  op_desc_ptr->AddInputDesc(1, tensor_desc_x2);
  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  op_desc_ptr->AddOutputDesc(tensor_desc_out);
  shared_ptr<Kernel> gathershapes_kernel = KernelFactory::Instance().Create("GatherShapes");
  ge::ComputeGraphPtr graph_ = std::make_shared<ge::ComputeGraph>("default");
  NodePtr node_ = graph_->AddNode(op_desc_ptr);
  std::vector<GeTensorPtr> v_output;
  ge::Status status = gathershapes_kernel->Compute(node_, v_output);
  EXPECT_EQ(ge::FAILED, status);
}