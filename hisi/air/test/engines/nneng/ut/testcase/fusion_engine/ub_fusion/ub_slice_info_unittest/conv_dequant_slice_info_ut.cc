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

#include <stdio.h>
#include "gtest/gtest.h"
#define protected public
#define private public
#include "graph_optimizer/ub_fusion/tbe_pass/ub_pass_slice_info/conv_dequant_slice_info.h"
#include "graph_optimizer/fusion_common/op_slice_info.h"
#include "graph_constructor.h"
#undef protected
#undef private
using namespace std;
using namespace fe;

class CONV_DEQUANT_SLICE_INFO_UTEST : public testing::Test {
 public:
 protected:
  virtual void SetUp() {
    conv_dequant_slice_info_ptr = std::make_shared<ConvDequantSliceInfo>();
  }

  virtual void TearDown() {

  }

  void BuildGraph_1(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data"})
        .SetInput("add", "dequant")
        .SetInput("quant", "add")
        .SetInput("stridedWrite", "quant");
  }

 public:
  std::shared_ptr<ConvDequantSliceInfo> conv_dequant_slice_info_ptr;
};


TEST_F(CONV_DEQUANT_SLICE_INFO_UTEST, modify_slice_info_by_pattern_suc) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_1(graph);
  ge::NodePtr fusion_node = graph->FindNode("dequant");
  const vector<ge::NodePtr> fusion_nodes;
  OpCalcInfo op_calc_info;
  op_calc_info.Initialize();
  size_t input_size = 1;
  bool is_head_fusion;
  Status ret = conv_dequant_slice_info_ptr->ModifySliceInfoByPattern(fusion_node, fusion_nodes, op_calc_info,
                                                                         input_size, is_head_fusion);
  EXPECT_EQ(fe::SUCCESS, ret);
}

TEST_F(CONV_DEQUANT_SLICE_INFO_UTEST, set_output_slice_info_for_requants16_suc) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_1(graph);
  ge::NodePtr fusion_node = graph->FindNode("dequant");
  OpCalcInfo op_calc_info;
  op_calc_info.Initialize();

  Status ret = conv_dequant_slice_info_ptr->SetOutputSliceInfoForEltwise(fusion_node, op_calc_info);
  EXPECT_EQ(fe::SUCCESS, ret);
  std::shared_ptr<UbPassSliceInfoBase> slice_info_base_ptr = std::make_shared<UbPassSliceInfoBase>();;
  ret = slice_info_base_ptr->ModifySliceInfoByPattern(fusion_node);
  EXPECT_EQ(fe::SUCCESS, ret);
}