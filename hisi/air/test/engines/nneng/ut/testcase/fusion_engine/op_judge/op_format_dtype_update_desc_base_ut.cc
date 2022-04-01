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
#include "graph_optimizer/op_judge/format_and_dtype/update_desc/op_format_dtype_update_desc_base.h"
#undef protected
#undef private

using namespace std;
using namespace fe;
using namespace ge;

class OP_FORMAT_DTYPE_UPDATE_DESC_BASE_UTEST: public testing::Test {
 protected:
  void SetUp() {
  }

  void TearDown() {
  }
};

TEST_F(OP_FORMAT_DTYPE_UPDATE_DESC_BASE_UTEST, PadNDToOtherFormatAndGetReshapeType_suc) {
  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr;
  FormatDtypeQuerierPtr format_dtype_querier_ptr = make_shared<FormatDtypeQuerier>(op_store_adapter_manager_ptr);
  OpFormatDtypeUpdateDescBase update_desc_base(format_dtype_querier_ptr);

  OpKernelInfoPtr op_kernel_info_ptr;             // op kernel info
  InputOrOutputInfoPtr input_or_output_info_ptr = make_shared<InputOrOutputInfo>("str_name");
  uint32_t matched_index;                         // mathed index of the op_kernel_info
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(4, 4);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape);
  op_desc_0->AddInputDesc(out_desc);
  op_desc_0->AddOutputDesc(out_desc);
  NodePtr node_ptr = graph->AddNode(op_desc_0);
  uint32_t index;                                 // the index of the input or output desc
  ge::GeTensorDesc op_input_or_output_desc;             // the input or output desc of the current node
  bool is_input;
  UpdateInfo update_info = {op_kernel_info_ptr, input_or_output_info_ptr, matched_index, node_ptr, index, op_input_or_output_desc, is_input};

  ge::Format op_kernel_format = ge::FORMAT_NC1HWC0;
  ge::Format ori_format;
  std::string reshape_type;
  Status ret = update_desc_base.PadNDToOtherFormatAndGetReshapeType(update_info, op_kernel_format, ori_format, reshape_type);
  EXPECT_EQ(ret, fe::SUCCESS);
}

// TEST_F(OP_FORMAT_DTYPE_UPDATE_DESC_BASE_UTEST, update_newshape_and_format) {
//   OpKernelInfoPtr op_kernel_info_ptr;
//   InputOrOutputInfoPtr input_or_output_info_ptr = make_shared<InputOrOutputInfo>("str_name");
//   uint32_t matched_index;
//   ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
//   OpDescPtr op_desc_0 = std::make_shared<OpDesc>("add", "Add");
//   vector<int64_t> dim(4, 4);
//   GeShape shape(dim);
//   GeTensorDesc out_desc(shape);
//   op_desc_0->AddInputDesc(out_desc);
//   op_desc_0->AddOutputDesc(out_desc);
//   NodePtr node_ptr = graph->AddNode(op_desc_0);
//   uint32_t index;
//   ge::GeTensorDesc op_input_or_output_desc;
//   bool is_input;
//   UpdateInfo update_info = {op_kernel_info_ptr, input_or_output_info_ptr, matched_index, node_ptr, index, op_input_or_output_desc, is_input}; 
//   ge::Format op_kernel_format = ge::FORMAT_FRACTAL_Z; 
//   int64_t group = 2;
//   ge::GeShape original_shape; 
//   ge::GeShape new_shape;
//   std::string op_type;
//   std::string op_name;
//   fe::UpdateNewShapeAndFormat(update_info, op_kernel_format, group, original_shape, new_shape, op_type, op_name);
// }