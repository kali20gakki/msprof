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


#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/compute_graph.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/compute_graph.h"

#define protected public
#define private public
#include "graph_optimizer/fe_graph_optimizer.h"
#include "graph_optimizer/graph_fusion/graph_matcher.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/avgpool_quant_process_fusion_pass.h"
#include "graph_optimizer/graph_fusion/fusion_pass_manager/builtin_pass/quant_pass/bias_optimize_quant_rollback/group_conv2d_quant_process_fusion_pass.h"
#include "../../../../graph_constructor/graph_constructor.h"
#include "common/unknown_shape_util.h"
#include "graph_optimizer/range_format_transfer/transfer_range_according_to_format.h"
#include "graph_optimizer/op_compiler/tbe_json_parse.h"
#undef private
#undef protected

using namespace std;
using namespace testing;
using namespace ge;
using namespace fe;

class UTestFusionEngineCoverage : public testing::Test {
 protected:
  void SetUp()
  {
  }

  void TearDown()
  {

  }

};


TEST_F(UTestFusionEngineCoverage, coverage5_1_option) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  GeTensorDesc data_desc_invalid({}, FORMAT_RESERVED, DT_UNDEFINED);
  GraphConstructor test(graph);
  test.AddOpDesc("A", "A", 1, 1);
  ge::NodePtr node = nullptr;
  test.GetNodeByName("A", node);

  for (auto &node : graph->GetDirectNode()) {
    if (node->GetOpDesc()->GetType() == "A") {
      node->GetOpDesc()->UpdateInputDesc(0, data_desc_invalid);
    }
  }

  ge::GeTensorDesc j;
  GroupConv2DQuantProcessFusionPass pass;

  vector<ge::NodePtr> fusionNodes;
  pass.BiasOptimize(*(graph.get()), node, node, node, fusionNodes);
}

TEST_F(UTestFusionEngineCoverage, coverage6) {
  vector<std::pair<int64_t, int64_t>> new_range;
  int64_t impl_type = 1;
  vector<std::pair<int64_t, int64_t>> range_value;
  vector<std::pair<int64_t, int64_t>> nd_range_value;

  vector<std::pair<int64_t, int64_t>> range_value_not_empty = {{1,1}};
  vector<std::pair<int64_t, int64_t>> nd_range_value_not_empty = {{1, 2}, {1, 2}};

  RangeTransferAccordingToFormat::GetNCHWRangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetNHWCRangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetNHWCRangeByAxisValue(new_range, impl_type, range_value_not_empty, nd_range_value);

  RangeTransferAccordingToFormat::GetNC1HWC0RangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetFzRangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetFzRangeByAxisValue(new_range, impl_type, range_value_not_empty, nd_range_value);

  RangeTransferAccordingToFormat::GetFzRangeByAxisValue(new_range, impl_type, range_value_not_empty, nd_range_value_not_empty);

  RangeTransferAccordingToFormat::GetFzC04RangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetHWCNRangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetHWCNRangeByAxisValue(new_range, impl_type, range_value_not_empty, nd_range_value);

  RangeTransferAccordingToFormat::GetCHWNRangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetCHWNRangeByAxisValue(new_range, impl_type, range_value_not_empty, nd_range_value);

  RangeTransferAccordingToFormat::GetC1HWNCoC0RangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetC1HWNCoC0RangeByAxisValue(new_range, impl_type, range_value_not_empty, nd_range_value);

  RangeTransferAccordingToFormat::GetNzRangeByAxisValue(new_range, impl_type, range_value, nd_range_value);

  RangeTransferAccordingToFormat::GetNzRangeByAxisValue(new_range, impl_type, range_value, range_value_not_empty);

  RangeTransferAccordingToFormat::GetNDC1HWC0RangeByAxisValue(new_range, impl_type, range_value, range_value_not_empty);

  RangeTransferAccordingToFormat::GetFz3DRangeByAxisValue(new_range, impl_type, range_value, range_value_not_empty);

  RangeTransferAccordingToFormat::GetFz3DTransposeRangeByAxisValue(new_range, impl_type, range_value, range_value_not_empty);

  ge::GeShape old_shape;
  vector<std::pair<int64_t, int64_t>> old_range;

  ge::Format old_format = ge::FORMAT_RESERVED;
  ge::Format new_format = old_format;
  ge::DataType current_data_type = ge::DT_UNDEFINED;
  int64_t op_impl_type = 1;
  RangeAndFormat range_and_format_info = {old_shape, old_range, new_range,
                                       old_format, new_format, current_data_type,
                                       op_impl_type};
  RangeTransferAccordingToFormat::GetRangeAccordingToFormat(range_and_format_info);

  RangeAndFormat range_and_format_info1 = {old_shape, old_range, new_range,
                                       ge::FORMAT_NC1HWC0, ge::FORMAT_NC1HWC0,
                                       current_data_type, op_impl_type};
  RangeTransferAccordingToFormat::GetRangeAccordingToFormat(range_and_format_info1);

}

TEST_F(UTestFusionEngineCoverage, coverage8) {
  ge::GeShape new_shape;
  int64_t impl_type = 1;
  vector<int64_t> axis_value;
  vector<int64_t> nd_value;

  vector<int64_t> nd_value_not_empty = {{1,1,3,4}};
  ShapeTransferAccordingToFormat::GetNCHWShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetNHWCShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetNC1HWC0ShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetFzShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetFzShapeByAxisValue(new_shape, impl_type, nd_value_not_empty, nd_value);

  ShapeTransferAccordingToFormat::GetFzC04ShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetHWCNShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetCHWNShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetC1HWNCoC0ShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetNzShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);

  ShapeTransferAccordingToFormat::GetNzShapeByAxisValue(new_shape, impl_type, axis_value, nd_value_not_empty);

  ShapeTransferAccordingToFormat::GetNDC1HWC0ShapeByAxisValue(new_shape, impl_type, axis_value, nd_value_not_empty);

  ShapeTransferAccordingToFormat::GetFz3DShapeByAxisValue(new_shape, impl_type, axis_value, nd_value_not_empty);

  ShapeTransferAccordingToFormat::GetFz3DTransposeShapeByAxisValue(new_shape, impl_type, axis_value, nd_value_not_empty);

  ge::GeShape old_shape;
  vector<std::pair<int64_t, int64_t>> old_range;

  ge::Format old_format = ge::FORMAT_RESERVED;
  ge::Format new_format = old_format;
  ge::DataType current_data_type = ge::DT_UNDEFINED;
  int64_t op_impl_type = 1;
  int64_t group_count = 0;
  ShapeAndFormat format_info = {old_shape, new_shape, old_format, new_format,
                               current_data_type, op_impl_type, group_count};
  ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(format_info);

  ShapeAndFormat format_info1 = {old_shape, new_shape, ge::FORMAT_NC1HWC0,
                               ge::FORMAT_NC1HWC0, current_data_type, op_impl_type,
                               group_count};
  ShapeTransferAccordingToFormat::GetShapeAccordingToFormat(format_info1);

}

TEST_F(UTestFusionEngineCoverage, coverage9) {
  OpKernelInfoConstructor constructor;
  OpContent op_content;
  OpKernelInfoPtr op_kernel_info = std::make_shared<OpKernelInfo>("A");
  AttrInfoPtr attr_info = std::make_shared<AttrInfo>("A1");
  attr_info->dtype_ = GeAttrValue::VT_INT;

  op_kernel_info->attrs_info_.emplace_back(attr_info);
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  op_kernel_info->attrs_info_[0]->dtype_ = GeAttrValue::VT_FLOAT;
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  op_kernel_info->attrs_info_[0]->dtype_ = GeAttrValue::VT_BOOL;
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  op_kernel_info->attrs_info_[0]->dtype_ = GeAttrValue::VT_STRING;
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  op_kernel_info->attrs_info_[0]->dtype_ = GeAttrValue::VT_LIST_INT;
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  op_kernel_info->attrs_info_[0]->dtype_ = GeAttrValue::VT_LIST_FLOAT;
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  op_kernel_info->attrs_info_[0]->dtype_ = GeAttrValue::VT_LIST_BOOL;
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  op_kernel_info->attrs_info_[0]->dtype_ = GeAttrValue::VT_BYTES;
  constructor.InitAttrValueSub(op_content, op_kernel_info);

  map<string, string> map_info;
  InputOrOutputInfoPtr input_or_output_info;
  uint32_t dtype_and_format_size_of_first_input;
  OpPattern op_pattern = OP_PATTERN_BROADCAST;
  constructor.InitDtypeByPattern(map_info, input_or_output_info,
                                 dtype_and_format_size_of_first_input, op_pattern);
}

TEST_F(UTestFusionEngineCoverage, GetFznRNNShapeByAxisValue_01) {
  ge::GeShape new_shape;
  int64_t impl_type;
  vector<int64_t> axis_value;
  vector<int64_t> nd_value;
  ShapeTransferAccordingToFormat::GetFznRNNShapeByAxisValue(new_shape, impl_type, axis_value, nd_value);
}
