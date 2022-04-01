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
#include <string>

#define private public
#include "opskernel/ops_kernel_info_types.h"
#include "graph/passes/same_transdata_breadth_fusion_pass.h"
#include "opskernel_manager/ops_kernel_manager.h"
#include "ge_graph_dsl/graph_dsl.h"
#include "init/gelib.h"
#include "graph/operator_reg.h"

using namespace ge;
using namespace std;

namespace {
REG_OP(Cast)
.INPUT(x, TensorType({DT_BOOL, DT_FLOAT16, DT_FLOAT, DT_INT8, DT_INT32, DT_UINT32, DT_UINT8, DT_INT64,
                      DT_UINT64, DT_INT16, DT_UINT16, DT_DOUBLE, DT_COMPLEX64, DT_COMPLEX128,
                      DT_QINT8, DT_QUINT8, DT_QINT16, DT_QUINT16, DT_QINT32})) /* input tensor */
.OUTPUT(y, TensorType({DT_BOOL, DT_FLOAT16, DT_FLOAT, DT_INT8, DT_INT32, DT_UINT32, DT_UINT8, DT_INT64,
                       DT_UINT64, DT_INT16, DT_UINT16, DT_DOUBLE, DT_COMPLEX64, DT_COMPLEX128,
                       DT_QINT8, DT_QUINT8, DT_QINT16, DT_QUINT16, DT_QINT32})) /* output tensor */
.ATTR(dst_type, Int, 0)
.ATTR(truncate, Bool, false)
.OP_END_FACTORY_REG(Cast)
}

class StubKernelInfoStore : public ge::OpsKernelInfoStore {
 public:
  Status Initialize(const std::map<std::string, std::string> &options) override {
    return 0U;
  }
  Status Finalize() override {return true;};
  void GetAllOpsKernelInfo(std::map<std::string, OpInfo> &infos) const override {}
  virtual bool CheckSupported(const OpDescPtr &opDescPtr, std::string &un_supported_reason) const override {
    return true;
  }
  bool CheckAccuracySupported(const OpDescPtr &opDescPtr, std::string &un_supported_reason,
                              bool realQuery = false) const override {
    return true;
  }
  bool CheckAccuracySupported(const ge::NodePtr &node, std::string &un_supported_reason,
                              bool realQuery = false) const override {
    return true;
  }
};

class UtestGraphPassesSameTransdataBreadthFusionPass : public testing::Test {
 public:
  static void SetUpTestCase() {
    auto instance = GELib::GetInstance();
    map<string, string> options;
    instance->Initialize(options);
    OpsKernelManager &ops_kernel_manager = instance->OpsKernelManagerObj();
    OpInfo op_info;
    op_info.opKernelLib = "test_support";
    ops_kernel_manager.ops_kernel_info_[TRANSDATA].emplace_back(op_info);
    ops_kernel_manager.ops_kernel_store_["test_support"] = std::make_shared<StubKernelInfoStore>();
  }
  static void TearDownTestCase() {
    auto instance = GELib::GetInstance();
    instance->Finalize();
  }
 protected:
  void SetUp() {}

  void TearDown() {}
};

class NodeBuilder {
 public:
  NodeBuilder(const std::string &name, const std::string &type) { op_desc_ = std::make_shared<OpDesc>(name, type); }

  NodeBuilder &AddInputDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                            ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddInputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  NodeBuilder &AddOutputDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                             ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddOutputDesc(CreateTensorDesc(shape, format, data_type)->Clone());
    return *this;
  }

  ge::NodePtr Build(const ge::ComputeGraphPtr &graph) { return graph->AddNode(op_desc_); }

 private:
  ge::GeTensorDescPtr CreateTensorDesc(std::initializer_list<int64_t> shape, ge::Format format = FORMAT_NCHW,
                                       ge::DataType data_type = DT_FLOAT) {
    GeShape ge_shape{std::vector<int64_t>(shape)};
    ge::GeTensorDescPtr tensor_desc = std::make_shared<ge::GeTensorDesc>();
    tensor_desc->SetShape(ge_shape);
    tensor_desc->SetFormat(format);
    tensor_desc->SetDataType(data_type);
    return tensor_desc;
  }

  ge::OpDescPtr op_desc_;
};
// Node4D(NCHW)->cast1(DT_BOOL->FP16)->transdata1(NCHW->NC1HWC0)->sinh1
//           /
//          --->cast2(DT_BOOL->FP16)->transdata2(NCHW->NC1HWC0)->sinh2
static void CreateGraph(ComputeGraphPtr &graph) {
  // Node4D
  ge::NodePtr node_data = NodeBuilder("Data4D", DATA).AddOutputDesc({2, 16, 2, 2}, FORMAT_NCHW, DT_BOOL).Build(graph);
  // cast1
  ge::NodePtr node_cast_1 = NodeBuilder("node_cast_1", CAST)
                                .AddInputDesc({2, 16, 2, 2}, FORMAT_NCHW, DT_BOOL)
                                .AddOutputDesc({2, 16, 2, 2}, FORMAT_NCHW, DT_FLOAT16)
                                .Build(graph);
  auto src_name = node_data->GetName();
  node_cast_1->GetOpDesc()->SetSrcName({src_name});
  node_cast_1->GetOpDesc()->SetInputName({src_name});
  AttrUtils::SetInt(node_cast_1->GetOpDesc(), CAST_ATTR_SRCT, DT_FLOAT);

  // trandata1
  ge::NodePtr node_transdata_1 = NodeBuilder("node_transdata_1", TRANSDATA)
                                     .AddInputDesc({2, 16, 2, 2}, FORMAT_NCHW, DT_FLOAT16)
                                     .AddOutputDesc({2, 1, 2, 2, 16}, FORMAT_NC1HWC0, DT_FLOAT16)
                                     .Build(graph);
  // sinh1
  ge::NodePtr node_sinh_1 = NodeBuilder("node_sinh_1", SINH)
                              .AddInputDesc({2, 1, 2, 2, 16}, FORMAT_NC1HWC0, DT_FLOAT16)
                              .AddOutputDesc({2, 1, 2, 2, 16}, FORMAT_NC1HWC0, DT_FLOAT16)
                              .Build(graph);
  // cast2
  ge::NodePtr node_cast_2 = NodeBuilder("node_cast_2", CAST)
                                .AddInputDesc({2, 16, 2, 2}, FORMAT_NCHW, DT_BOOL)
                                .AddOutputDesc({2, 16, 2, 2}, FORMAT_NCHW, DT_FLOAT16)
                                .Build(graph);
  node_cast_2->GetOpDesc()->SetSrcName({src_name});
  node_cast_2->GetOpDesc()->SetInputName({src_name});
  // transdata2
  ge::NodePtr node_transdata_2 = NodeBuilder("node_transdata_2", TRANSDATA)
                                     .AddInputDesc({2, 16, 2, 2}, FORMAT_NCHW, DT_FLOAT16)
                                     .AddOutputDesc({2, 1, 2, 2, 16}, FORMAT_NC1HWC0, DT_FLOAT16)
                                     .Build(graph);

  // sinh2
  ge::NodePtr node_sinh_2 = NodeBuilder("node_sinh_2", SINH)
                              .AddInputDesc({2, 1, 2, 2, 16}, FORMAT_NC1HWC0, DT_FLOAT16)
                              .AddOutputDesc({2, 1, 2, 2, 16}, FORMAT_NC1HWC0, DT_FLOAT16)
                              .Build(graph);

  // add edge
  ge::GraphUtils::AddEdge(node_data->GetOutDataAnchor(0), node_cast_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_cast_1->GetOutDataAnchor(0), node_transdata_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_transdata_1->GetOutDataAnchor(0), node_sinh_1->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_data->GetOutDataAnchor(0), node_cast_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_cast_2->GetOutDataAnchor(0), node_transdata_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_transdata_2->GetOutDataAnchor(0), node_sinh_2->GetInDataAnchor(0));
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_unsupported_transdata_succ_1) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test1");
  (void)CreateGraph(graph);

  ge::SameTransdataBreadthFusionPass pass;
  ge::graphStatus status = pass.Run(graph);
  EXPECT_EQ(ge::GRAPH_SUCCESS, status);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_unsupported_transdata_succ_2) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test2");
  (void)CreateGraph(graph);
  ge::SameTransdataBreadthFusionPass pass;
  ge::graphStatus status = pass.Run(graph);
  EXPECT_EQ(ge::GRAPH_SUCCESS, status);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_unsupported_transdata_succ_3) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test3");
  (void)CreateGraph(graph);
  ge::SameTransdataBreadthFusionPass pass;
  ge::graphStatus status = pass.Run(graph);
  EXPECT_EQ(ge::GRAPH_SUCCESS, status);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_same_transop_fusion_pass_not_cause_loop_unittest) {
  DEF_GRAPH(g1) {
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("cast1", CAST));
    CHAIN(NODE("data1", DATA)->EDGE(0, 0)->NODE("cast2", CAST));
    CHAIN(NODE("cast1", CAST)->EDGE(0, 0)->NODE("transdata1", TRANSDATA));
    CHAIN(NODE("cast2", CAST)->EDGE(0, 0)->NODE("transdata2", TRANSDATA));
    CHAIN(NODE("transdata1", TRANSDATA)->EDGE(0, 0)->NODE("transdata3", TRANSDATA));
    CHAIN(NODE("transdata3", TRANSDATA)->EDGE(0, 0)->NODE("netoutput1", NETOUTPUT));
    CHAIN(NODE("transdata2", TRANSDATA)->EDGE(0, 0)->NODE("abs", ABSVAL));
    CHAIN(NODE("abs", ABSVAL)->CTRL_EDGE()->NODE("cast1", CAST));
    CHAIN(NODE("abs2", ABSVAL)->CTRL_EDGE()->NODE("transdata2", TRANSDATA));
    CHAIN(NODE("abs1", ABSVAL)->CTRL_EDGE()->NODE("transdata1", TRANSDATA));
  };
  auto graph = ToComputeGraph(g1);

  ge::SameTransdataBreadthFusionPass pass;
  ge::graphStatus status = pass.Run(graph);
  EXPECT_EQ(ge::GRAPH_SUCCESS, status);
  EXPECT_EQ(graph->TopologicalSorting(), SUCCESS);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata_2_cast_2) {
  /*
            |---transdata---cast---A
            |
    Node4D--|---transdata---cast---B

            ||
            \/
                            |---cast---A
                            |
    Node4D--|---transdata---|---cast---B
  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_5d_2_4hd_1 = NodeBuilder("5d_2_4d_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  ge::NodePtr node_5d_2_4hd_2 = NodeBuilder("5d_2_4d_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_softmax_2 = NodeBuilder("softemax_2", SOFTMAX)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_5d_2_4hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_5d_2_4hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_5d_2_4hd_1->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_5d_2_4hd_2->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), node_softmax_2->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata_no_out) {
  /*
            |---cast--transdata---A
            |
    Node4D--|---transdata---B
            |
            |---cast---transdata

            ||
            \/

                            |---cast---A
                            |
    Node4D--|---transdata---|---B
            |
            |---cast---transdata

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_3 = NodeBuilder("4d_2_5hd_3", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NHWC, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), node_4d_2_5hd_3->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 2);
  EXPECT_EQ(node_4d_2_5hd_2->GetOutDataNodes().size(), 2);
  EXPECT_EQ(cast_fp32_2_fp16_2->GetOutDataNodes().size(), 1);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata2_cast_multi_out) {
  /*
                      |---C
                      |
            |---cast--|---transdata---A
            |
    Node4D--|---cast---cast---transdata---B

            ||
            \/

                            |---cast---A
                            |
    Node4D--|---transdata---|---cast---cast---B
            |
            |---cast---C

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr cast_fp16_2_int8 = NodeBuilder("cast_fp16_2_int8", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), cast_fp16_2_int8->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_relu_3->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 2);
  EXPECT_EQ(node_4d_2_5hd_1->GetOutDataNodes().size(), 2);
  EXPECT_EQ(cast_fp32_2_fp16_1->GetOutDataNodes().size(), 1);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_no_transdata) {
  /*
            |---cast---transpose---A
            |
    Node4D--|---transpose---B
            |
            |---C

            ||
            \/

            |---cast---transpose---A
            |
    Node4D--|---transpose---B
            |
            |---C

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);

  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr transpose_1 = NodeBuilder("transpose_1", TRANSPOSE)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 3, 4, 2}, FORMAT_NHWC, DT_FLOAT16)
      .Build(graph);


  ge::NodePtr transpose_2 = NodeBuilder("transpose_2", TRANSPOSE)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 3, 4, 2}, FORMAT_NHWC, DT_FLOAT)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), transpose_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_relu_3->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), transpose_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(transpose_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_relu_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 3);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_different_transdata) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---transdata---B
            |
            |---transdata---C

            ||
            \/

            |---cast---transdata---A
            |
    Node4D--|---transdata---|---cast---B
                            |---C

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);

  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);


  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_FRACTAL_Z, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_3 = NodeBuilder("4d_2_5hd_3", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_2_5hd_3->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_4d_2_5hd_3->GetOutDataAnchor(0), node_relu_3->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 2);
  EXPECT_EQ(node_4d_2_5hd_3->GetOutDataNodes().size(), 2);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata2_cast1_simple) {
  /*

            |---cast---transdata---A
    Node4D--|
            |---transdata---B
  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);

  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16 = NodeBuilder("cast_4d_fp32_2_fp16", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);


  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(cast_fp32_2_fp16->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_2->GetOutDataNodes().size(), 2);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata3_cast2_simple) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---transdata---B
            |
            |---transdata---C

            ||
            \/

            |               |---cast---A
            |               |
    Node4D--|---transdata---|---cast---B
                            |---C

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);

  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);


  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_3 = NodeBuilder("4d_2_5hd_3", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), node_4d_2_5hd_3->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_4d_2_5hd_3->GetOutDataAnchor(0), node_relu_3->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_3->GetOutDataNodes().size(), 3);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata3_cast2_anchors_out3) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---transdata---B
            |
            |---transdata---C

            ||
            \/

            |---cast---transdata---A
            |
    Node4D--|---cast---transdata---B
            |
            |---transdata---C
  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);

  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);


  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_3 = NodeBuilder("4d_2_5hd_3", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(1), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(2), node_4d_2_5hd_3->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_4d_2_5hd_3->GetOutDataAnchor(0), node_relu_3->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);

  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 3);
  EXPECT_EQ(node_4d_2_5hd_1->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_2->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_3->GetOutDataNodes().size(), 1);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata2_cast3) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---cast---transdata---B

            ||
            \/

                        |---cast---A
                        |
    Node4D--transdata---|---cast---cast---B

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr cast_fp16_2_int8 = NodeBuilder("cast_fp16_2_int8", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), cast_fp16_2_int8->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_1->GetOutDataNodes().size(), 2);
}


TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata3_cast4) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---cast---transdata---B
            |
            |---cast---transdata---C
            ||
            \/

                        |---cast---A
                        |
    Node4D--transdata---|---cast---cast---B
                        |
                        |---cast---C

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_3 = NodeBuilder("cast_4d_fp32_2_fp16_3", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp16_2_int8 = NodeBuilder("cast_fp16_2_int8", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_3 = NodeBuilder("4d_2_5hd_3", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_3->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), cast_fp16_2_int8->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_3->GetOutDataAnchor(0), node_4d_2_5hd_3->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_3->GetOutDataAnchor(0), node_relu_3->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_3->GetOutDataNodes().size(), 3);
}


TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_cast3_two_out) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---cast---transdata---B
            |           |
            |           |---C
            ||
            \/

                           |---cast---A
                           |
    Node4D--|--transdata---|---cast---cast---B
            |
            |---cast---cast---C

  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp16_2_int8 = NodeBuilder("cast_fp16_2_int8", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), cast_fp16_2_int8->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_relu_3->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 2);
  EXPECT_EQ(node_4d_2_5hd_1->GetOutDataNodes().size(), 2);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata_out_control) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---cast---transdata---B
                                       |
                                       |---C
            ||
            \/

                            |---cast---A
                            |
    Node4D--|---transdata---|---cast---cast---B
                                          |
                                          |---C


  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp16_2_int8 = NodeBuilder("cast_fp16_2_int8", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), cast_fp16_2_int8->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutControlAnchor(), node_relu_3->GetInControlAnchor());

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_1->GetOutDataNodes().size(), 2);
  EXPECT_EQ(cast_fp16_2_int8->GetOutControlNodes().size(), 1);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata_out_data_control) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---cast---transdata---B
                                      |
                                      |---C

            ||
            \/

                            |---cast---A
                            |
    Node4D--|---transdata---|---cast---cast---B
                                          |
                                          |---C


  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp16_2_int8 = NodeBuilder("cast_fp16_2_int8", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), cast_fp16_2_int8->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_3->GetInControlAnchor());

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_1->GetOutDataNodes().size(), 2);
  EXPECT_EQ(cast_fp16_2_int8->GetOutControlNodes().size(), 1);
}

TEST_F(UtestGraphPassesSameTransdataBreadthFusionPass, test_transdata_out_control_peer_in_data) {
  /*
            |---cast---transdata---A
            |
    Node4D--|---cast---cast---transdata---B
                                      |
                                      |---C

            ||
            \/

                            |---cast---A
                            |
    Node4D--|---transdata---|---cast---cast---B
                                          |
                                          |---C


  */
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  // Node4D
  ge::NodePtr node_4d = NodeBuilder("node_4d", DATA)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .Build(graph);
  // cast fp32 -> fp16
  ge::NodePtr cast_fp32_2_fp16_1 = NodeBuilder("cast_4d_fp32_2_fp16_1", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp32_2_fp16_2 = NodeBuilder("cast_4d_fp32_2_fp16_2", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr cast_fp16_2_int8 = NodeBuilder("cast_fp16_2_int8", CAST)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .Build(graph);

  ge::NodePtr node_4d_2_5hd_1 = NodeBuilder("4d_2_5hd_1", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);
  ge::NodePtr node_4d_2_5hd_2 = NodeBuilder("4d_2_5hd_2", TRANSDATA)
      .AddInputDesc({1, 2, 3, 4}, FORMAT_NCHW, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_1 = NodeBuilder("relu_1", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_FLOAT16)
      .Build(graph);

  ge::NodePtr node_relu_2 = NodeBuilder("relu_2", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::NodePtr node_relu_3 = NodeBuilder("relu_3", RELU)
      .AddInputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .AddOutputDesc({1, 2, 3, 4, 5}, FORMAT_NC1HWC0, DT_INT8)
      .Build(graph);

  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d->GetOutDataAnchor(0), cast_fp32_2_fp16_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_1->GetOutDataAnchor(0), node_4d_2_5hd_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_1->GetOutDataAnchor(0), node_relu_1->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp32_2_fp16_2->GetOutDataAnchor(0), cast_fp16_2_int8->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(cast_fp16_2_int8->GetOutDataAnchor(0), node_4d_2_5hd_2->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutDataAnchor(0), node_relu_2->GetInDataAnchor(0));

  ge::GraphUtils::AddEdge(node_4d_2_5hd_2->GetOutControlAnchor(), node_relu_3->GetInDataAnchor(0));

  ge::SameTransdataBreadthFusionPass pass;
  Status status = pass.Run(graph);
  EXPECT_EQ(status, SUCCESS);
  EXPECT_EQ(node_4d->GetOutDataNodes().size(), 1);
  EXPECT_EQ(node_4d_2_5hd_1->GetOutDataNodes().size(), 2);
  EXPECT_EQ(cast_fp16_2_int8->GetOutControlNodes().size(), 1);
}