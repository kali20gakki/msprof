/**
 * Copyright 2019-2021 Huawei Technologies Co., Ltd
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
#include "graph/passes/constant_folding_pass.h"

#include <string>
#include <vector>

#include "common/types.h"
#include "common/plugin/ge_util.h"
#include "graph/passes/base_pass.h"
#include "graph/passes/dimension_compute_pass.h"
#include "graph_builder_utils.h"
#include "inc/kernel.h"
#include "inc/kernel_factory.h"
#include "graph/utils/constant_utils.h"
#include "init/gelib.h"
#undef protected
#undef private

namespace ge {
const char *AddYesDim = "AddYesDim";
const char *AddNYes = "AddNYes";
const char *AddNNo = "AddNNo";
const char *AddYes = "AddYes";
const char *HuberLossYes = "HuberLossYes";
const char *ShapeNo = "ShapeNo";
const char *DataNo = "dataNo";
const char *WrongYes = "WrongYes";
const char *WrongYes1 = "WrongYes1";
const char *WrongYes2 = "WrongYes2";
const char *WrongYes3 = "WrongYes3";
const char *WhereDynamic2Static = "WhereDynamic2Static";

class TestAddNKernel : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    auto output = std::make_shared<GeTensor>();
    std::vector<uint8_t> data{1, 2, 3};
    std::vector<int64_t> shape{3};
    output->MutableTensorDesc().SetShape(GeShape(shape));
    output->SetData(data);
    output->MutableTensorDesc().SetDataType(DT_UINT8);
    v_output.push_back(output);
    return SUCCESS;
  }
};
REGISTER_KERNEL(AddNYes, TestAddNKernel);

class TestWhereKernel : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    auto output = std::make_shared<GeTensor>();
    std::vector<uint8_t> data{1, 2, 3};
    std::vector<int64_t> shape{3};
    output->MutableTensorDesc().SetShape(GeShape(shape));
    output->SetData(data);
    output->MutableTensorDesc().SetDataType(DT_UINT8);
    v_output.push_back(output);
    return SUCCESS;
  }
};
REGISTER_KERNEL(WhereDynamic2Static, TestWhereKernel);

class TestHuberLossKernel : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    auto output1 = std::make_shared<GeTensor>();
    std::vector<uint8_t> data{1, 2, 3, 4, 5};
    std::vector<int64_t> shape{5};
    output1->MutableTensorDesc().SetShape(GeShape(shape));
    output1->SetData(data);
    output1->MutableTensorDesc().SetDataType(DT_UINT8);
    v_output.push_back(output1);

    auto output2 = std::make_shared<GeTensor>();
    std::vector<uint8_t> data2{1, 2, 3, 4, 5, 6};
    std::vector<int64_t> shape2{2, 3};
    output2->MutableTensorDesc().SetShape(GeShape(shape2));
    output2->SetData(data2);
    output2->MutableTensorDesc().SetDataType(DT_UINT8);
    v_output.push_back(output2);

    return SUCCESS;
  }
};
REGISTER_KERNEL(HuberLossYes, TestHuberLossKernel);

class TestAddKernel : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    auto output = std::make_shared<GeTensor>();
    std::vector<uint8_t> data{1, 2, 3, 4, 5};
    std::vector<int64_t> shape{5};
    output->MutableTensorDesc().SetShape(GeShape(shape));
    output->SetData(data);
    output->MutableTensorDesc().SetDataType(DT_UINT8);
    v_output.push_back(output);
    return SUCCESS;
  }
};
REGISTER_KERNEL(AddYes, TestAddKernel);

class TestAddDimKernel : public Kernel {
 public:
  Status Compute(const ge::NodePtr &node, std::vector<ge::GeTensorPtr> &v_output) const override {
    auto output = std::make_shared<GeTensor>();
    std::vector<uint8_t> data{1, 2, 3, 4, 5};
    std::vector<int64_t> shape{5};
    output->MutableTensorDesc().SetShape(GeShape(shape));
    output->SetData(data);
    output->MutableTensorDesc().SetDataType(DT_UINT8);
    v_output.push_back(output);
    return SUCCESS;
  }
};
REGISTER_KERNEL(AddYesDim, TestAddDimKernel);

class TestWrongKernel : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    // for test: output weights is null
    v_output.push_back(nullptr);
    return SUCCESS;
  }
};
REGISTER_KERNEL(WrongYes, TestWrongKernel);

class TestWrongKernel1 : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    // for test: no output weights
    return SUCCESS;
  }
};
REGISTER_KERNEL(WrongYes1, TestWrongKernel1);

class TestWrongKernel2 : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    auto output1 = std::make_shared<GeTensor>();
    std::vector<uint8_t> data{1, 2, 3, 4, 5};
    std::vector<int64_t> shape{5};
    output1->MutableTensorDesc().SetShape(GeShape(shape));
    output1->SetData(data);
    output1->MutableTensorDesc().SetDataType(DT_UINT8);
    v_output.push_back(output1);
    // for test: output weights < output size
    return SUCCESS;
  }
};
REGISTER_KERNEL(WrongYes2, TestWrongKernel2);

class TestWrongKernel3 : public Kernel {
 public:
  Status Compute(const ge::OpDescPtr op_desc_ptr, const std::vector<ge::ConstGeTensorPtr> &input,
                 std::vector<ge::GeTensorPtr> &v_output) override {
    // for test: return NOT_CHANGED
    return NOT_CHANGED;
  }
};
REGISTER_KERNEL(WrongYes3, TestWrongKernel3);

class UtestGraphPassesConstantFoldingPass : public testing::Test {
 protected:
  UtestGraphPassesConstantFoldingPass() = default;
};

namespace {
void SetWeightForConstNode(NodePtr &const_node) {
  // new a tensor
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);
  ConstantUtils::SetWeight(const_node->GetOpDesc(), 0, tensor);
}

///     netoutput1
///        |
///      shapeNo1
///       |
///     addnYes1
///    /    \.
///  /       \.
/// const1   const2
ComputeGraphPtr BuildGraph1() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);
  (void)AttrUtils::SetStr(addn1->GetOpDesc(), ATTR_NAME_STREAM_LABEL, "stream_label");

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///       |
///     addnYes1  shapeNo2
///    /    \     /
///  /       \  /
/// const1   const2
ComputeGraphPtr BuildGraph2() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto shape2 = builder.AddNode("shape2", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", DataNo, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddDataEdge(const2, 0, shape2, 0);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///       |         c
///     addnYes1  <-----  dataNo1
///    /    \.
///  /       \.
/// const1   const2
ComputeGraphPtr BuildGraph3() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto data1 = builder.AddNode("data1", DataNo, 0, 1);
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddControlEdge(data1, addn1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///       |         c
///     addnYes1  <---------
///    /    \               \.
///  /       \         c     \.
/// const1   const2  <-----  dataNo1
ComputeGraphPtr BuildGraph4() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto data1 = builder.AddNode("data1", DataNo, 0, 1);
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);
  
  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddControlEdge(data1, const2);
  builder.AddControlEdge(data1, addn1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///       |         c
///     addnYes1  <-----  dataNo1
///    /    \.
///  /       \        c
/// const1   const2  <-----  dataNo2
ComputeGraphPtr BuildGraph5() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto data1 = builder.AddNode("data1", DataNo, 0, 1);
  auto data2 = builder.AddNode("data2", DataNo, 0, 1);
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddControlEdge(data2, const2);
  builder.AddControlEdge(data1, addn1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///        |
///     addYes1  <---- const3
///        |
///     addnYes1 <-
///    /    \      \.
///  /       \      \.
/// const1   const2  const4
ComputeGraphPtr BuildGraph6() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto const3 = builder.AddNode("const3", CONSTANT, 0, 1);
  auto const4 = builder.AddNode("const4", CONSTANT, 0, 1);
  auto addn1 = builder.AddNode("addn1", AddNYes, 3, 1);
  auto add1 = builder.AddNode("add1", AddYes, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);
  SetWeightForConstNode(const3);
  SetWeightForConstNode(const4);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddDataEdge(const4, 0, addn1, 2);
  builder.AddDataEdge(addn1, 0, add1, 0);
  builder.AddDataEdge(const3, 0, add1, 1);
  builder.AddDataEdge(add1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///         netoutput1
///          /       \.
///    shapeNo1     ShpaeNo2
///         \      /
///      huberLoss1
///    /      |    \.
///  /       |      \.
/// const1  const2  const3
ComputeGraphPtr BuildGraph7() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto const3 = builder.AddNode("const3", CONSTANT, 0, 1);
  auto huberLoss1 = builder.AddNode("huberLoss1", HuberLossYes, 3, 2);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto shape2 = builder.AddNode("shape2", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);
  SetWeightForConstNode(const3);

  builder.AddDataEdge(const1, 0, huberLoss1, 0);
  builder.AddDataEdge(const2, 0, huberLoss1, 1);
  builder.AddDataEdge(const3, 0, huberLoss1, 2);
  builder.AddDataEdge(huberLoss1, 0, shape1, 0);
  builder.AddDataEdge(huberLoss1, 1, shape2, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);
  builder.AddDataEdge(shape2, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///       |
///     addnNo1
///    /    \.
///  /       \.
/// const1   const2
ComputeGraphPtr BuildGraph8() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto addn1 = builder.AddNode("addn1", AddNNo, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(const2, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      shapeNo1
///       |
///     addnYes1
///    /    \.
///  /       \.
/// const1   data1
ComputeGraphPtr BuildGraph9() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto data1 = builder.AddNode("data1", DataNo, 0, 1);
  auto addn1 = builder.AddNode("addn1", AddNYes, 2, 1);
  auto shape1 = builder.AddNode("shape1", ShapeNo, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);

  builder.AddDataEdge(const1, 0, addn1, 0);
  builder.AddDataEdge(data1, 0, addn1, 1);
  builder.AddDataEdge(addn1, 0, shape1, 0);
  builder.AddDataEdge(shape1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///    netoutput1
///     /      \.
///  addDim   sqrt1
///     \      /
///     switch1
///     /    \.
///    /      \.
///  const1  const2
ComputeGraphPtr BuildGraph10() {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  auto const2 = builder.AddNode("const2", CONSTANT, 0, 1);
  auto switchNode1 = builder.AddNode("switch1", SWITCH, 2, 2);
  auto sqrt1 = builder.AddNode("sqrt1", RSQRT, 1, 1);
  auto add1 = builder.AddNode("addDim", AddYesDim, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  SetWeightForConstNode(const1);
  SetWeightForConstNode(const2);

  builder.AddDataEdge(const1, 0, switchNode1, 0);
  builder.AddDataEdge(const2, 0, switchNode1, 1);
  builder.AddDataEdge(switchNode1, 0, add1, 0);
  builder.AddDataEdge(switchNode1, 1, sqrt1, 0);
  builder.AddDataEdge(add1, 0, netoutput1, 0);
  builder.AddDataEdge(sqrt1, 0, netoutput1, 0);

  return builder.GetGraph();
}

///     netoutput1
///        |
///      FRAMEWORKOP
///        |
///        const1
ComputeGraphPtr BuildWrongGraph1() {
  auto builder = ut::GraphBuilder("test");
  auto const_op = builder.AddNode("const1", CONSTANT, 0, 1);
  SetWeightForConstNode(const_op);
  auto op = builder.AddNode("fmk_op", FRAMEWORKOP, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  builder.AddDataEdge(const_op, 0, op, 0);
  builder.AddDataEdge(op, 0, netoutput1, 0);
  return builder.GetGraph();
}

///     netoutput1
///        |
///      WrongYes
///         |
///        const1
ComputeGraphPtr BuildWrongGraph2() {
  auto builder = ut::GraphBuilder("test");
  auto const_op = builder.AddNode("const1", CONSTANT, 0, 1);
  SetWeightForConstNode(const_op);
  auto op = builder.AddNode("wrong", WrongYes, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  builder.AddDataEdge(const_op, 0, op, 0);
  builder.AddDataEdge(op, 0, netoutput1, 0);
  return builder.GetGraph();
}

///     netoutput1
///        |
///      WrongYes1
///         |
///        const1
ComputeGraphPtr BuildWrongGraph3() {
  auto builder = ut::GraphBuilder("test");
  auto const_op = builder.AddNode("const1", CONSTANT, 0, 1);
  SetWeightForConstNode(const_op);
  auto op = builder.AddNode("wrong1", WrongYes1, 1, 1);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  builder.AddDataEdge(const_op, 0, op, 0);
  builder.AddDataEdge(op, 0, netoutput1, 0);
  return builder.GetGraph();
}

///  netoutput1  WrongYes1
///        |     /
///      WrongYes2
///         /
///       const1
ComputeGraphPtr BuildWrongGraph4() {
  auto builder = ut::GraphBuilder("test");
  auto const_op_1 = builder.AddNode("const1", CONSTANT, 0, 1);
  SetWeightForConstNode(const_op_1);
  auto op = builder.AddNode("wrong2", WrongYes2, 1, 2);
  auto netoutput1 = builder.AddNode("netoutput", NETOUTPUT, 1, 0);
  auto wrong_op = builder.AddNode("WrongYes1", WrongYes1, 1, 0);
  builder.AddDataEdge(const_op_1, 0, op, 0);
  builder.AddDataEdge(op, 0, netoutput1, 0);
  builder.AddDataEdge(op, 1, wrong_op, 0);
  return builder.GetGraph();
}

///   CONVOLUTION
///        |
///      WrongYes2  WrongYes1
///         /
///       const1
ComputeGraphPtr BuildWrongGraph5() {
  auto builder = ut::GraphBuilder("test");
  auto const_op_1 = builder.AddNode("const1", CONSTANT, 0, 1);
  SetWeightForConstNode(const_op_1);
  auto op = builder.AddNode("wrong2", WrongYes2, 1, 1);
  auto conv = builder.AddNode("conv", CONVOLUTION, 1, 0);
  auto wrong_op = builder.AddNode("WrongYes1", WrongYes1, 1, 0);
  builder.AddDataEdge(const_op_1, 0, op, 0);
  builder.AddDataEdge(op, 0, conv, 0);
  return builder.GetGraph();
}

///   CONVOLUTION
///        |
///      WrongYes3
///         /
///       const1
ComputeGraphPtr BuildWrongGraph6() {
  auto builder = ut::GraphBuilder("test");
  auto const_op_1 = builder.AddNode("const1", CONSTANT, 0, 1);
  SetWeightForConstNode(const_op_1);
  auto op = builder.AddNode("wrong3", WrongYes3, 1, 1);
  auto conv = builder.AddNode("conv", CONVOLUTION, 1, 0);
  builder.AddDataEdge(const_op_1, 0, op, 0);
  builder.AddDataEdge(op, 0, conv, 0);
  return builder.GetGraph();
}
///   data1  data2
///     \   /
///      add
///       |
///   netoutput
///
ComputeGraphPtr BuildWrongGraph7() {
  auto builder = ut::GraphBuilder("g5");
  auto data1 = builder.AddNode("input1", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  auto data2 = builder.AddNode("input2", DATA, 1, 1, FORMAT_NCHW, DT_FLOAT, {4, 10});
  auto add = builder.AddNode("add", ADD, 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(data1, 0, add, 0);
  builder.AddDataEdge(data2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

///   const1 const2
///     \   /
///      add(ATTR_NO_NEED_CONSTANT_FOLDING)
///       |
///   netoutput
///
ComputeGraphPtr BuildWrongGraph8() {
  auto builder = ut::GraphBuilder("g5");
  auto const1 = builder.AddNode("input1", CONSTANT, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  auto const2 = builder.AddNode("input2", CONSTANT, 1, 1, FORMAT_NCHW, DT_FLOAT, {4, 10});
  auto add = builder.AddNode("add", ADD, 2, 1);
  (void)AttrUtils::SetBool(add->GetOpDesc(), ATTR_NO_NEED_CONSTANT_FOLDING, true);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(const1, 0, add, 0);
  builder.AddDataEdge(const2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

/// shape1(pc)  shape2(potential_const)
///         \   /
///          add
///           |
///        netoutput
///
ComputeGraphPtr BuildPotentialConstGraph() {
  // new a tensor
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  auto builder = ut::GraphBuilder("g5");
  auto shape1 = builder.AddNode("shape1", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape1->GetOpDesc(), {0}, {tensor});
  auto shape2 = builder.AddNode("shape2", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape2->GetOpDesc(), {0}, {tensor});
  auto add = builder.AddNode("add", AddYes, 2, 1);
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(shape1, 0, add, 0);
  builder.AddDataEdge(shape2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

///  shape1(pc)  shape2(pc)
///     \        /
///      add(potential_const)
///       |
///   netoutput
///
ComputeGraphPtr BuildPotentialConstGraph2() {
  // new a tensor
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  auto builder = ut::GraphBuilder("g5");
  auto shape1 = builder.AddNode("shape1", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape1->GetOpDesc(), {0}, {tensor});
  auto shape2 = builder.AddNode("shape2", SHAPE, 1, 1, FORMAT_NCHW, DT_FLOAT, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape2->GetOpDesc(), {0}, {tensor});
  auto add = builder.AddNode("add", WrongYes3, 2, 1);
  ConstantUtils::MarkPotentialConst(add->GetOpDesc(), {0}, {tensor});
  AttrUtils::SetStr(add->GetOpDesc(), "_source_pass_of_potential_const", "ConstantFoldingPass"); // mark potential from constant folding pass
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0);

  builder.AddDataEdge(shape1, 0, add, 0);
  builder.AddDataEdge(shape2, 0, add, 1);
  builder.AddDataEdge(add, 0,netoutput, 0);
  return builder.GetGraph();
}

///    shape1(pc)
///       |
///      where
///       |
///   netoutput
///
ComputeGraphPtr BuildPotentialConstGraph3() {
  // new a tensor
  ge::GeTensorPtr tensor = std::make_shared<GeTensor>();
  std::vector<uint8_t> value{1, 2, 3};
  std::vector<int64_t> shape{3};
  tensor->MutableTensorDesc().SetShape(GeShape(shape));
  tensor->SetData(value);
  tensor->MutableTensorDesc().SetDataType(DT_UINT8);

  auto builder = ut::GraphBuilder("g5");
  auto shape1 = builder.AddNode("shape1", SHAPE, 1, 1, FORMAT_NCHW, DT_UINT8, {1, 2, 3});
  ConstantUtils::MarkPotentialConst(shape1->GetOpDesc(), {0}, {tensor});
  auto where = builder.AddNode("where", WhereDynamic2Static, 1, 1, FORMAT_NCHW, DT_UINT8, {-1});
  auto netoutput = builder.AddNode("netoutput", NETOUTPUT, 1, 0, FORMAT_NCHW, DT_UINT8, {-1});

  builder.AddDataEdge(shape1, 0, where, 0);
  builder.AddDataEdge(where, 0, netoutput, 0);
  return builder.GetGraph();
}
}  // namespace

TEST_F(UtestGraphPassesConstantFoldingPass, folding_addn) {
  auto graph = BuildGraph1();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 3);
  auto shape1 = graph->FindNode("shape1");
  EXPECT_NE(shape1, nullptr);
  EXPECT_EQ(shape1->GetInNodes().size(), 1);

  auto folded_const = shape1->GetInDataNodes().at(0);
  EXPECT_EQ(folded_const->GetType(), CONSTANT);
  auto tensor = folded_const->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ(tensor.GetDataType(), DT_UINT8);
  EXPECT_EQ(tensor.GetShape().GetDims(), std::vector<int64_t>({3}));

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, folding_without_one_const) {
  auto graph = BuildGraph2();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 5);
  EXPECT_EQ(graph->FindNode("addn1"), nullptr);
  EXPECT_EQ(graph->FindNode("const1"), nullptr);

  auto const2 = graph->FindNode("const2");
  EXPECT_NE(const2, nullptr);
  EXPECT_EQ(const2->GetOutDataNodes().size(), 1);
  EXPECT_EQ(const2->GetOutDataNodes().at(0)->GetName(), "shape2");

  auto shape1 = graph->FindNode("shape1");
  EXPECT_NE(shape1, nullptr);
  EXPECT_EQ(shape1->GetInDataNodes().size(), 1);
  EXPECT_EQ(shape1->GetInDataNodes().at(0)->GetType(), CONSTANT);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, folding_with_const_control_edges) {
  auto graph = BuildGraph5();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 5);
  auto shape1 = graph->FindNode("shape1");
  EXPECT_NE(shape1, nullptr);
  EXPECT_EQ(shape1->GetInNodes().size(), 1);
  EXPECT_EQ(shape1->GetInControlNodes().size(), 0);
  EXPECT_EQ(shape1->GetInDataNodes().at(0)->GetType(), CONSTANT);
  std::unordered_set<std::string> node_names;
  for (auto node : shape1->GetInControlNodes()) {
    node_names.insert(node->GetName());
  }
  EXPECT_EQ(node_names, std::unordered_set<std::string>());

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, continues_fold) {
  auto graph = BuildGraph6();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 3);
  auto shape1 = graph->FindNode("shape1");
  EXPECT_NE(shape1, nullptr);
  EXPECT_EQ(shape1->GetInNodes().size(), 1);

  auto folded_const = shape1->GetInDataNodes().at(0);
  EXPECT_EQ(folded_const->GetType(), CONSTANT);
  auto tensor = folded_const->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ(tensor.GetDataType(), DT_UINT8);
  EXPECT_EQ(tensor.GetShape().GetDims(), std::vector<int64_t>({5}));

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, multiple_output) {
  auto graph = BuildGraph7();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);

  EXPECT_EQ(graph->GetAllNodes().size(), 5);

  auto shape1 = graph->FindNode("shape1");
  EXPECT_NE(shape1, nullptr);
  EXPECT_EQ(shape1->GetInNodes().size(), 1);
  auto folded_const = shape1->GetInDataNodes().at(0);
  EXPECT_EQ(folded_const->GetType(), CONSTANT);
  auto tensor = folded_const->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ(tensor.GetDataType(), DT_UINT8);
  EXPECT_EQ(tensor.GetShape().GetDims(), std::vector<int64_t>({5}));

  auto shape2 = graph->FindNode("shape2");
  EXPECT_NE(shape2, nullptr);
  EXPECT_EQ(shape2->GetInNodes().size(), 1);
  auto folded_const2 = shape2->GetInDataNodes().at(0);
  EXPECT_EQ(folded_const2->GetType(), CONSTANT);
  auto tensor2 = folded_const2->GetOpDesc()->GetOutputDesc(0);
  EXPECT_EQ(tensor2.GetDataType(), DT_UINT8);
  EXPECT_EQ(tensor2.GetShape().GetDims(), std::vector<int64_t>({2, 3}));

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, not_change1) {
  auto graph = BuildGraph8();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);

  EXPECT_EQ(graph->GetAllNodes().size(), 5);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, not_change2) {
  auto graph = BuildGraph9();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 5);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, folding_size) {
  auto graph = BuildGraph10();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new DimensionComputePass});

  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 7);

  auto switchnode = graph->FindNode("switch1");
  EXPECT_NE(switchnode, nullptr);
  EXPECT_EQ(switchnode->GetOutDataNodes().size(), 2);
  EXPECT_EQ(switchnode->GetOutDataNodes().at(0)->GetName(), "addDim_ctrl_identity_0");

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, unlikely1) {
  auto graph = BuildWrongGraph1();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, unlikely2) {
  auto graph = BuildWrongGraph2();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), INTERNAL_ERROR);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, unlikely3) {
  auto graph = BuildWrongGraph3();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), INTERNAL_ERROR);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}
TEST_F(UtestGraphPassesConstantFoldingPass, unlikely4) {
  auto graph = BuildWrongGraph4();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), INTERNAL_ERROR);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}
TEST_F(UtestGraphPassesConstantFoldingPass, unlikely5) {
  auto graph = BuildWrongGraph5();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}
TEST_F(UtestGraphPassesConstantFoldingPass, unlikely6) {
  auto graph = BuildWrongGraph6();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);

  for (auto &name_to_pass : names_to_pass) {
    delete name_to_pass.second;
  }
}

TEST_F(UtestGraphPassesConstantFoldingPass, not_constant_input_ignore_pass) {
  auto graph = BuildWrongGraph7();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 4);
}

TEST_F(UtestGraphPassesConstantFoldingPass, attr_need_ignore_pass) {
  auto graph = BuildWrongGraph8();
  ConstantFoldingPass constant_folding_pass;
  auto add_node = graph->FindNode("add");
  EXPECT_TRUE(constant_folding_pass.NeedIgnorePass(add_node));
}

TEST_F(UtestGraphPassesConstantFoldingPass, test_run_mark_potential_const) {
  auto graph = BuildPotentialConstGraph();
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 4);
  auto add_node = graph->FindNode("add");
  EXPECT_EQ(ConstantUtils::IsPotentialConst(add_node->GetOpDesc()), true);
  ConstGeTensorPtr potential_weight;
  EXPECT_EQ(ConstantUtils::GetWeight(add_node->GetOpDesc(), 0, potential_weight), true);
  vector<int64_t> shape = {5};
  EXPECT_EQ(potential_weight->GetTensorDesc().GetShape().GetDims(), shape);
}

TEST_F(UtestGraphPassesConstantFoldingPass, test_run_unmark_potential_const) {
  // mark potential const on add node
  auto graph = BuildPotentialConstGraph2();
  auto add_node = graph->FindNode("add");
  EXPECT_TRUE(ConstantUtils::IsPotentialConst(add_node->GetOpDesc()));
  NamesToPass names_to_pass;
  ConstantFoldingPass constant_folding_pass;
  names_to_pass.push_back({"Test", &constant_folding_pass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  constant_folding_pass.GetGeConstantFoldingPerfStatistic();
  constant_folding_pass.GetOpConstantFoldingPerfStatistic();
  EXPECT_EQ(graph->GetAllNodes().size(), 4);
  EXPECT_FALSE(ConstantUtils::IsPotentialConst(add_node->GetOpDesc()));
}

TEST_F(UtestGraphPassesConstantFoldingPass, test_kernel_computed_shape_changed_on_potential) {
  // mark potential const on add node
  auto graph = BuildPotentialConstGraph3();
  auto where_node = graph->FindNode("where");
  EXPECT_FALSE(ConstantUtils::IsPotentialConst(where_node->GetOpDesc()));
  auto netoutput_node = graph->FindNode("netoutput");
  EXPECT_EQ(netoutput_node->GetOpDesc()->GetInputDesc(0).GetShape().GetDim(0), -1);
  NamesToPass names_to_pass;
  names_to_pass.push_back({"Test", new ConstantFoldingPass});
  GEPass pass(graph);
  EXPECT_EQ(pass.Run(names_to_pass), SUCCESS);
  EXPECT_EQ(graph->GetAllNodes().size(), 3);
  EXPECT_TRUE(ConstantUtils::IsPotentialConst(where_node->GetOpDesc()));
  EXPECT_EQ(netoutput_node->GetOpDesc()->GetInputDesc(0).GetShape().GetDim(0), 3);
}

TEST_F(UtestGraphPassesConstantFoldingPass, test_collect_cost_time_Of_OpConstant_Folding) {
  auto builder = ut::GraphBuilder("test");
  auto const1 = builder.AddNode("const1", CONSTANT, 0, 1);
  ConstantFoldingPass pass;
  // insert
  pass.CollectCostTimeOfOpConstantFolding(const1, 1);
  pass.CollectCostTimeOfOpConstantFolding(const1, 1);
  auto map = pass.GetOpConstantFoldingPerfStatistic();
  EXPECT_EQ(map.empty(), false);
}

TEST_F(UtestGraphPassesConstantFoldingPass, testComputeWithHostCpuKernel) {
  ConstantFoldingPass pass;
  NodePtr node;
  vector<ConstGeTensorPtr> inputs;
  std::vector<GeTensorPtr> outputs;
  map<string, string> options;
  Status ret = ge::GELib::Initialize(options);
  EXPECT_EQ(ret, SUCCESS);
  ret = pass.ComputeWithHostCpuKernel(node, inputs, outputs);
  EXPECT_EQ(ret, UNSUPPORTED);
}

}  // namespace ge
