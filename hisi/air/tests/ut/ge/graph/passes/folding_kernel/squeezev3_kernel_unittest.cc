/**
 * Copyright 2022Huawei Technologies Co., Ltd
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
#include "host_kernels/squeezev3_kernel.h"

#include "../graph_builder_utils.h"
#include "common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/fp16_t.h"
#include "common/ge_inner_error_codes.h"
#include "common/op/ge_op_utils.h"
#include "common/types.h"
#include "graph/types.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "inc/kernel_factory.h"
#undef protected
#undef private

using namespace testing;
using namespace ge;

class UtestFoldingKernelSqueezeV3Kernel : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

namespace{

void TestValidSqueezeV3(vector<int64_t> &data_vec, vector<int8_t> &dim_value_vec) {

  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  ge::OpDescPtr data_op_desc = std::make_shared<ge::OpDesc>("data", CONSTANTOP);
  int64_t dims_size = 1;
  for_each(data_vec.begin(), data_vec.end(), [&](int64_t &data) { dims_size *= data; });
  vector<int8_t> data_value_vec(dims_size, 2);
  GeTensorDesc data_tensor_desc(GeShape(data_vec), FORMAT_NCHW, ge::DT_INT8);
  auto data_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec.data(),
                                                       data_value_vec.size() * sizeof(int8_t));
  OpDescUtils::SetWeights(data_op_desc, data_tensor);
  data_op_desc->AddOutputDesc(data_tensor_desc);
  NodePtr data_node = graph->AddNode(data_op_desc);
  data_node->Init();

  // add axes node
  ge::OpDescPtr dim_op_desc = std::make_shared<ge::OpDesc>("axes", CONSTANTOP);
  GeTensorDesc dim_tensor_desc(ge::GeShape(), FORMAT_NCHW, ge::DT_INT8);
  auto dim_tensor = std::make_shared<GeTensor>(dim_tensor_desc, (uint8_t *)dim_value_vec.data(),
                                                      dim_value_vec.size() * sizeof(int8_t));
  OpDescUtils::SetWeights(dim_op_desc, dim_tensor);
  dim_op_desc->AddOutputDesc(dim_tensor_desc);
  NodePtr dim_node = graph->AddNode(dim_op_desc);
  dim_node->Init();

  // add SqueezeV3 node
  OpDescPtr squeezev3_op_desc = std::make_shared<OpDesc>("SqueezeV3", SQUEEZEV3);
  squeezev3_op_desc->AddInputDesc(data_tensor_desc);
  squeezev3_op_desc->AddInputDesc(dim_tensor_desc);
  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  squeezev3_op_desc->AddOutputDesc(tensor_desc_out);
  auto node_ptr = graph->AddNode(squeezev3_op_desc);
  node_ptr->Init();

  // add edge
  GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), node_ptr->GetInDataAnchor(0));
  GraphUtils::AddEdge(dim_node->GetOutDataAnchor(0), node_ptr->GetInDataAnchor(1));

  Status status;
  auto kernel = KernelFactory::Instance().Create(SQUEEZEV3);
  status = kernel->Compute(node_ptr);
  EXPECT_EQ(ge::SUCCESS, status);

  std::vector<GeTensorPtr> v_output;
  std::vector<ConstGeTensorPtr> input = {data_tensor, dim_tensor};
  status = kernel->Compute(node_ptr->GetOpDesc(), input, v_output);
  EXPECT_EQ(ge::SUCCESS, status);
  EXPECT_EQ(1, v_output.size());
  int8_t *data_buf = (int8_t *)v_output[0]->GetData().data();
  for (size_t i = 0; i < 6; i++) {
    EXPECT_EQ(*(data_buf + i), 2);
  }
}

void TestInvalidSqueezeV3(vector<int64_t> &data_vec, vector<int8_t> &dim_value_vec) {
  ge::ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("default");

  ge::OpDescPtr data_op_desc = std::make_shared<ge::OpDesc>("data", CONSTANTOP);
  int64_t dims_size = 1;
  for_each(data_vec.begin(), data_vec.end(), [&](int64_t &data) { dims_size *= data; });
  vector<int8_t> data_value_vec(dims_size, 2);
  GeTensorDesc data_tensor_desc(GeShape(data_vec), FORMAT_NCHW, ge::DT_INT8);
  auto data_tensor = std::make_shared<GeTensor>(data_tensor_desc, (uint8_t *)data_value_vec.data(),
                                                       data_value_vec.size() * sizeof(int8_t));
  OpDescUtils::SetWeights(data_op_desc, data_tensor);
  data_op_desc->AddOutputDesc(data_tensor_desc);
  NodePtr data_node = graph->AddNode(data_op_desc);
  data_node->Init();

  // add axes node
  ge::OpDescPtr dim_op_desc = std::make_shared<ge::OpDesc>("axes", CONSTANTOP);
  GeTensorDesc dim_tensor_desc(ge::GeShape(), FORMAT_NCHW, ge::DT_INT8);
  auto dim_tensor = std::make_shared<GeTensor>(dim_tensor_desc, (uint8_t *)dim_value_vec.data(),
                                                      dim_value_vec.size() * sizeof(int8_t));
  OpDescUtils::SetWeights(dim_op_desc, dim_tensor);
  dim_op_desc->AddOutputDesc(dim_tensor_desc);
  NodePtr dim_node = graph->AddNode(dim_op_desc);
  dim_node->Init();

  // add SqueezeV3 node
  OpDescPtr squeezev3_op_desc = std::make_shared<OpDesc>("SqueezeV3", SQUEEZEV3);
  squeezev3_op_desc->AddInputDesc(data_tensor_desc);
  squeezev3_op_desc->AddInputDesc(dim_tensor_desc);

  auto node_ptr = graph->AddNode(squeezev3_op_desc);
  node_ptr->Init();

  Status status;
  auto kernel = KernelFactory::Instance().Create(SQUEEZEV3);
  status = kernel->Compute(node_ptr);
  EXPECT_EQ(ge::NOT_CHANGED, status);


  std::vector<GeTensorPtr> v_output;
  std::vector<ConstGeTensorPtr> input = {data_tensor, dim_tensor};
  status = kernel->Compute(node_ptr->GetOpDesc(), input, v_output);
  EXPECT_EQ(ge::NOT_CHANGED, status);

  // add edge
  GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), node_ptr->GetInDataAnchor(0));
  GraphUtils::AddEdge(dim_node->GetOutDataAnchor(0), node_ptr->GetInDataAnchor(1));

  GeTensorDesc tensor_desc_out(GeShape(), FORMAT_NCHW, DT_INT32);
  squeezev3_op_desc->AddOutputDesc(tensor_desc_out);
  std::vector<ConstGeTensorPtr> input_2 = {nullptr, nullptr};
  status = kernel->Compute(node_ptr->GetOpDesc(), input_2, v_output);
  EXPECT_EQ(ge::PARAM_INVALID, status);
}
} //namespace

TEST_F(UtestFoldingKernelSqueezeV3Kernel, ValidFoldingTest) {
  vector<int64_t> data_vec = {1, 2, 3};
  vector<int8_t> dim_value_vec = {0};
  TestValidSqueezeV3(data_vec, dim_value_vec);
}

TEST_F(UtestFoldingKernelSqueezeV3Kernel, InvalidFoldingTest) {
  vector<int64_t> data_vec = {1, 2, 3};
  vector<int8_t> dim_value_vec = {0};
  TestInvalidSqueezeV3(data_vec, dim_value_vec);
}