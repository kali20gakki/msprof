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

#include <stdio.h>
#include <map>
#include <memory>
#include "gtest/gtest.h"
#include "proto/om.pb.h"
#define protected public
#define private public
#include "common/graph_comm.h"
#include "common/pass_manager.h"
#include "common/configuration.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_tensor.h"
#include "graph/op_desc.h"
#include "graph/op_kernel_bin.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/ub_fusion/buffer_fusion.h"
#include "graph_optimizer/ub_fusion/automatic_buffer_fusion.h"
#include "graph_optimizer/ub_fusion/tbe_pass/tbe_common_rules0_fusion_pass.h"

#include "../../../graph_constructor/graph_constructor.h"

#undef protected
#undef private
using namespace std;
using namespace domi;
using namespace fe;
using namespace ge;

class UB_FUSION_ST_COMMON_RULES0 : public testing::Test {
 public:
 protected:

  static void SetUpTestCase() { std::cout << "UB fusion SetUp" << std::endl; }

  static void TearDownTestCase() {
    std::cout << "UB fusion TearDown" << std::endl;
  }

  virtual void SetUp() {
    graph_comm_ptr_ = std::make_shared<GraphComm>("engineName");
    graph_comm_ptr_->Initialize();
    fusion_pass_mgr_ptr_ = std::make_shared<FusionPassManager>();
    fusion_priority_mgr_ptr_ = std::make_shared<FusionPriorityManager>(
                                       "engineName", fusion_pass_mgr_ptr_,nullptr);
    sub_graph_optimizer_ptr_ = std::make_shared<BufferFusion>(graph_comm_ptr_,
                  fusion_pass_mgr_ptr_, fusion_priority_mgr_ptr_);
  }

  virtual void TearDown() {

 }

  std::shared_ptr<GraphComm> graph_comm_ptr_;
  std::shared_ptr<FusionPassManager> fusion_pass_mgr_ptr_;
  std::shared_ptr<FusionPriorityManager> fusion_priority_mgr_ptr_;
  std::shared_ptr<BufferFusion> sub_graph_optimizer_ptr_;

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

  void BuildGraph_depthwise1(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "DepthwiseConvolution", "depthwise", "DepthwiseConv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("depthwise", "stridedRead")
        .SetInputs("dequant", {"depthwise", "Data"})
        .SetInput("add", "dequant")
        .SetInput("quant", "add")
        .SetInput("stridedWrite", "quant");
  }

  void BuildGraph_2(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInputs("dequant", {"conv", "Data"})
        .SetInput("add", "dequant")
        .SetInput("quant", "add")
        .SetInput("stridedWrite", "quant");
  }

  void BuildGraph_depthwise2(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "DepthwiseConvolution", "depthwise", "DepthwiseConv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInputs("dequant", {"depthwise", "Data"})
        .SetInput("add", "dequant")
        .SetInput("quant", "add")
        .SetInput("stridedWrite", "quant");
  }

  void BuildGraph_3(ge::ComputeGraphPtr &graph) {
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
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("quant", "add")
        .SetInput("stridedWrite", "quant")
        .SetInput("Data_1", "stridedWrite:0")
        .SetInput("Data_2", "stridedWrite:0");
  }

  void BuildGraph_4(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "pRelu", "PRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "leakyRelu", "LeakyRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("eltwise", "add")
        .SetInput("relu6", "eltwise")
        .SetInput("pRelu", "relu6")
        .SetInput("leakyRelu", "pRelu")
        .SetInput("quant", "leakyRelu")
        .SetInput("stridedWrite", "quant");
  }

  void BuildGraph_5(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "pRelu", "PRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "leakyRelu", "LeakyRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul", "Mul", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("eltwise", "add")
        .SetInput("relu6", "eltwise")
        .SetInput("pRelu", "relu6")
        .SetInput("leakyRelu", "pRelu")
        .SetInput("mul", "leakyRelu")
        .SetInput("quant", "mul")
        .SetInput("stridedWrite", "quant");
  }

  void BuildGraph_6(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("eltwise", "add")
        .SetInput("quant", "eltwise");
  }

  void BuildGraph_7(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("quant", "add")
        .SetInput("stridedWrite", "quant:0")
        .SetInput("Data_1", "quant:0");
  }

  void BuildGraph_8(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("stridedWrite", "add");
  }

  void BuildGraph_9(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("quant", "dequant")
        .SetInput("stridedWrite", "quant");
  }

  void BuildGraph_10(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("stridedWrite", "dequant");
  }

  void BuildGraph_11(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("quant", "dequant");
  }

  void BuildGraph_12(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInput("quant", "conv");
  }

  void BuildGraph_13(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("stridedWrite", "add:0")
        .SetInput("Data_1", "add:0");
  }

  void BuildGraph_14(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "pRelu", "PRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("pRelu", "dequant")
        .SetInput("add", "pRelu")
        .SetInput("stridedWrite", "add:0")
        .SetInput("Data_1", "add:0");
  }

  void BuildGraph_15(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "pRelu", "PRelu", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("pRelu", "dequant");
  }

  void BuildGraph_16(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "pRelu", "PRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "leakyRelu", "LeakyRelu", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("eltwise", "add")
        .SetInput("relu6", "eltwise")
        .SetInput("pRelu", "relu6")
        .SetInput("leakyRelu", "pRelu")
        .SetInput("Data_1", "leakyRelu");
  }

  void BuildGraph_17(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu6", "Relu6", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "pRelu", "PRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "leakyRelu", "LeakyRelu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "mul", "Mul", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant")
        .SetInput("eltwise", "add")
        .SetInput("relu6", "eltwise")
        .SetInput("pRelu", "relu6")
        .SetInput("leakyRelu", "pRelu")
        .SetInput("mul", "leakyRelu")
        .SetInput("Data_1", "mul");
  }

  void BuildGraph_18(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("add", "dequant:0")
        .SetInput("eltwise", "dequant:0");
  }

  void BuildGraph_19(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInput("Data", "dequant");
  }

  void BuildGraph_20(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInputs("add", {"dequant", "eltwise"})
        .SetInput("Data_1", "add");
  }

  void BuildGraph_21(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "pRelu", "PRelu", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInputs("dequant", {"conv", "Data_0"})
        .SetInputs("add", {"dequant", "eltwise"})
        .SetInputs("pRelu", {"add", "Data_1"})
        .SetInput("Data_2", "pRelu");
  }

  void BuildGraph_22(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead");
  }

  void BuildGraph_23(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInput("conv", "stridedRead")
        .SetInput("Data_0", "conv:0")
        .SetInput("Data_1", "conv:0");
  }

  void BuildGraph_24(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "strided_read", "stridedRead", "StridedRead", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 2, 1)
        .SetInput("stridedRead", "Data_0")
        .SetInputs("conv", {"stridedRead", "Data_0"})
        .SetInput("Data_1", "conv");
  }

  void BuildGraph_25(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .SetInput("Data_1", "conv");
  }

  void BuildGraph_26(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "dequant", "dequant", "AscendDequant", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "strided_write", "stridedWrite", "StridedWrite", 1, 1)
        .SetInputs("dequant", {"conv", "Data"})
        .SetInput("add", "dequant")
        .SetInput("quant", "add")
        .SetInput("stridedWrite", "quant");
  }

  void BuildGraph_27(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "sigmoid", "sigmoid", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .SetInput("sigmoid", "conv")
        .SetInput("quant", "sigmoid");
  }

  void BuildGraph_28(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "sigmoid", "sigmoid", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .SetInput("add", "conv")
        .SetInput("sigmoid", "add")
        .SetInput("eltwise", "sigmoid")
        .SetInput("quant", "eltwise");
  }

  void BuildGraph_30(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", "AscendQuant", 1, 1)
        .SetInput("quant", "conv");
  }

  void BuildGraph_31(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "softplus", "Softplus", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 1, 1)
        .SetInput("softplus", "conv")
        .SetInput("add", "softplus");
  }

  void BuildGraph_32(ge::ComputeGraphPtr &graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "softplus", "addWW", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "read_select", "readSelect", "ReadSelect", 1, 1)
        .SetInput("softplus", "conv")
        .SetInput("readSelect", "softplus");
  }

  void BuildGraph_33(ComputeGraphPtr& graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "softplus", "addWW", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "sigmoid", "sigmoid", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "Eltwise", 2, 1)
        .SetInput("softplus", "conv")
        .SetInput("eltwise", "softplus", {3, 12, 5, 6})
        .SetInput("eltwise", "sigmoid", {3, 12, 8, 6});
  }

  void BuildGraph_34(ComputeGraphPtr& graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
        original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "softplus", "addWW", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "sigmoid", "sigmoid", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "eltwise", "ReluGradV2", 2, 1)
        .SetInput("softplus", "conv")
        .SetInput("eltwise", "softplus", {3, 12, 5, 6})
        .SetInput("eltwise", "sigmoid", {3, 12, 8, 6});
  }

  void BuildGraph_35(ComputeGraphPtr& graph) {
    ge::GeShape original_shape = ge::GeShape({3, 12, 5, 6});
    GraphConstructor test(graph, "", ge::FORMAT_NHWC, ge::DT_FLOAT,
                          original_shape);
    test.AddOpDesc(EN_IMPL_HW_TBE, "Convolution", "conv", "Conv2D", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "min1", "Minimum", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "relu", "Relu", 1, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "add", "Add", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "ElemWise", "min2", "Minimum", 2, 1)
        .AddOpDesc(EN_IMPL_HW_TBE, "quant", "quant", ASCEND_QUANT, 1, 1)
        .SetInput("min1:0", "conv:0")
        .SetInput("min1:1", "Data_0:0")

        .SetInput("relu:0", "min1:0")
        .SetInput("add:0", "min1:0")
        .SetInput("add:1", "Data_0:0")

        .SetInput("min2:0", "add:0")
        .SetInput("min2:1", "Data_0:0")

        .SetInput("NetOutput:0", "relu:0")

        .SetInput("quant:0", "min2:0")
        .SetInput("NetOutput:1", "quant:0");
  }

  void BuildGraph_36(ComputeGraphPtr& graph) {
    ge::OpDescPtr data_desc = std::make_shared<OpDesc>("data", "Data");
    ge::OpDescPtr stridedread_desc = std::make_shared<OpDesc>("stridedread", "StridedRead");
    ge::OpDescPtr conv_desc = std::make_shared<OpDesc>("conv2d", "Conv2D");
    ge::OpDescPtr data1_desc = std::make_shared<OpDesc>("data1", "Data");
    ge::OpDescPtr data2_desc = std::make_shared<OpDesc>("data2", "Data");
    ge::OpDescPtr stridedwrite_desc = std::make_shared<OpDesc>("stridedwrite", "StridedWrite");
    ge::OpDescPtr out_desc = std::make_shared<OpDesc>("netoutput", "NetOutput");

    ge::AttrUtils::SetStr(stridedread_desc, stridedread_desc->GetName() + "_pattern", "strided_read");
    ge::AttrUtils::SetStr(conv_desc, conv_desc->GetName() + "_pattern", "Convolution");
    ge::AttrUtils::SetStr(stridedwrite_desc, stridedwrite_desc->GetName() + "_pattern", "strided_write");
    AttrUtils::SetInt(stridedread_desc, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(conv_desc, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    AttrUtils::SetInt(stridedwrite_desc, FE_IMPLY_TYPE, fe::EN_IMPL_HW_TBE);
    ge::AttrUtils::SetInt(stridedread_desc, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
    ge::AttrUtils::SetInt(conv_desc, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
    ge::AttrUtils::SetInt(stridedwrite_desc, ge::ATTR_NAME_IMPLY_TYPE,static_cast<int64_t>(domi::ImplyType::TVM));
    AttrUtils::SetInt(stridedread_desc, "stride", 18);
    AttrUtils::SetInt(stridedwrite_desc, "stride", 18);

    const char tbe_bin[] = "tbe_bin";
    vector<char> buffer(tbe_bin, tbe_bin + strlen(tbe_bin));
    ge::OpKernelBinPtr tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(stridedread_desc->GetName(), std::move(buffer));
    stridedread_desc->SetExtAttr(OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);

    ge::GeShape original_shape = ge::GeShape({3, 1, 5, 5, 16});
    GeTensorDesc tensorDesc = GeTensorDesc(original_shape, ge::FORMAT_NC1HWC0, ge::DT_FLOAT16);
    data_desc->AddOutputDesc(tensorDesc);
    data1_desc->AddOutputDesc(tensorDesc);
    data2_desc->AddOutputDesc(tensorDesc);
    stridedread_desc->AddInputDesc(tensorDesc);
    stridedread_desc->AddOutputDesc(tensorDesc);
    conv_desc->AddInputDesc(tensorDesc);
    conv_desc->AddInputDesc(tensorDesc);
    conv_desc->AddInputDesc(tensorDesc);
    conv_desc->AddOutputDesc(tensorDesc);
    stridedwrite_desc->AddInputDesc(tensorDesc);
    stridedwrite_desc->AddOutputDesc(tensorDesc);
    out_desc->AddInputDesc(tensorDesc);

    ge::NodePtr data_node = graph->AddNode(data_desc);
    ge::NodePtr data1_node = graph->AddNode(data1_desc);
    ge::NodePtr data2_node = graph->AddNode(data2_desc);
    ge::NodePtr stridedread_node = graph->AddNode(stridedread_desc);
    ge::NodePtr conv_node = graph->AddNode(conv_desc);
    ge::NodePtr stridedwrite_node = graph->AddNode(stridedwrite_desc);
    ge::NodePtr out_node = graph->AddNode(out_desc);

    ge::GraphUtils::AddEdge(data_node->GetOutDataAnchor(0), stridedread_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(stridedread_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(data1_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(data2_node->GetOutDataAnchor(0), conv_node->GetInDataAnchor(2));
    ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0), stridedwrite_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(stridedwrite_node->GetOutDataAnchor(0), out_node->GetInDataAnchor(0));
  }
};

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_1) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_1(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantaddquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_depthwise1) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_depthwise1(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReaddepthwisedequantaddquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_2) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_2(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantaddquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_depthwise2) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_depthwise2(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "DepthwiseConv2D" &&
        node->GetName() == "depthwisedequantaddquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_3) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_3(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantaddquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_4) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_4(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantaddeltwiserelu6pReluleakyReluquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_5) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_5(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantaddeltwiserelu6pReluleakyRelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_6) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_6(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantaddeltwisequant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_7) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_7(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantaddquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_8) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_8(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantaddstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_9) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_9(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_10) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_10(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_11) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_11(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_12) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_12(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_13) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_13(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantadd") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_14) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_14(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantpReluadd") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_15) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_15(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantpRelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_16) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_16(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantaddeltwiserelu6pReluleakyRelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_17) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_17(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantaddeltwiserelu6pReluleakyRelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_18) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_18(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_19) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_19(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_20) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_20(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantadd") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_21) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_21(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconvdequantaddpRelu") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_22) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_22(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconv") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_23) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_23(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconv") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_24) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_24(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "StridedRead" &&
        node->GetName() == "stridedReadconv") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_25) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_25(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "conv") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_26) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_26(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convdequantaddquantstridedWrite") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_27) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_27(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "conv") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_28) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_28(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convadd") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_30) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_30(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convquant") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_31) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_31(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convsoftplusadd") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_32) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_32(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convsoftplus") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_33) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_33(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

  uint32_t id = 0;
  int find = 0;
  cerr << endl;
  cerr << "UB_FUSION_ST_COMMON_RULES0 UB fusion result" << endl;
  for (auto &node : graph->GetDirectNode()) {
    cerr << "id:" << id << endl;
    uint32_t scope_id = 0;
    cerr << "name: " << node->GetName() <<
      ", type:" << node->GetOpDesc()->GetType() << endl;
    cerr << "name:" <<  node->GetName() << endl;
    if (node->GetOpDesc()->GetType() == "Conv2D" &&
        node->GetName() == "convsoftplus") {
      find = 1;
    }
    if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
      cerr << "scope id : " << scope_id << endl;
    }
    id++;
  }
  EXPECT_EQ(find, 1);
}

TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_34) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_34(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);
  std::vector<ge::NodePtr> elemwise_nodes;
  ge::NodePtr node = nullptr;
  for (auto &node : graph->GetDirectNode()) {
    elemwise_nodes.push_back(node);
  }
  auto ret = TbeCommonRules0FusionPass::IsInBlackListOfOpPatternElemwise(elemwise_nodes, node);
}


TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_35) {
  ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
  BuildGraph_35(graph);

  sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
  sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
  sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);
  std::vector<ge::NodePtr> elemwise_nodes;
  ge::NodePtr node = nullptr;
  uint32_t count_fusion_op = 0;
  for (auto &node : graph->GetDirectNode()) {
    if (node->GetName() == "convmin1addmin2quant") {
      count_fusion_op++;
    }
  }
  EXPECT_EQ(count_fusion_op, 1);
}


TEST_F(UB_FUSION_ST_COMMON_RULES0, common_rules0_36) {
ComputeGraphPtr graph = std::make_shared<ge::ComputeGraph>("test");
BuildGraph_36(graph);

sub_graph_optimizer_ptr_->engine_name_ = fe::AI_CORE_NAME;
sub_graph_optimizer_ptr_->MatchFusionPatternFromGraph(*graph);
sub_graph_optimizer_ptr_->BuildFusionGraph(*graph);

uint32_t id = 0;
int find = 0;
cerr << endl;
cerr << "UB_FUSION_UT_COMMON_RULES0 UB fusion result" << endl;
for (auto &node : graph->GetDirectNode()) {
cerr << "id:" << id << endl;
uint32_t scope_id = 0;
cerr << "name: " << node->GetName() <<
", type:" << node->GetOpDesc()->GetType() << endl;
cerr << "name:" <<  node->GetName() << endl;
if (node->GetOpDesc()->GetType() == "StridedRead" && node->GetName() == "stridedreadconv2dstridedwrite") {
find = 1;
}
if (AttrUtils::GetInt(node->GetOpDesc(), SCOPE_ID_ATTR, scope_id)) {
cerr << "scope id : " << scope_id << endl;
}
id++;
}
EXPECT_EQ(find, 1);
}