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
#include "graph_optimizer/node_optimizer/concat_c_to_n_optimizer.h"
#include "graph_optimizer/fe_graph_optimizer.h"
#include "common/fe_utils.h"
#include "common/pass_manager.h"
#include "common/util/op_info_util.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph/debug/ge_attr_define.h"

#undef protected
#undef private

using namespace std;
using namespace ge;
using namespace fe;

namespace fe {
using FEGraphOptimizerPtr = std::shared_ptr<fe::FEGraphOptimizer>;

class UTEST_concat_c_to_n_optimize : public testing::Test {
 public:
  const string GRAPH_NAME = "test";
  const string CONCATD = "ConcatD";
  const string DEQUANT = "AscendDequant";
  const string CONV2D = "Conv2D";
  const string RELU = "Relu";
  const string LEAKYRELU = "LeakyRelu";
  const string STRIDEDWRITE = "StridedWrite";
  const string STRIDEDREAD = "StridedRead";
  const string STRIDE_ATTR_STRIDE = "stride";
  const string STRIDE_ATTR_AXIS = "axis";
  const int NCHW_DIM_N = 1;
  const int NCHW_DIM_H = 14;
  const int NCHW_DIM_W = 12;

protected:
  void SetUp() {}
  void TearDown() {}
  void InitGraph1(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, -1, 14, 14});
    ge::GeShape shape2({1, -1, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NC1HWC0, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);

    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=0)
     *          |
     */
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

    void InitGraph2(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 0);

    // add descriptor
    ge::GeShape shape1({1, 16, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);

    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=0)
     *          |
     */
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  void InitGraph3(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 16, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    (void)ge::AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(conv2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);

    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);

    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=1)
     *          |
     */
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  void InitGraph4(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 0);

    // add descriptor
    ge::GeShape shape1({1, 16, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    (void)ge::AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(conv2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);


    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);

    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=1)
     *          |
     */
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  void InitGraph5(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({2, 16, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);
    (void)ge::AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(conv2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
  
    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=1)
     *          |
     */
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  void InitGraph6(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 16, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    conv1->AddOutputDesc(out_desc1);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc1);
    (void)ge::AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);


    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=1)
     *          |
     */
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  void InitGraph7(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr read = std::make_shared<OpDesc>("read", STRIDEDREAD);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 16, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    ge::GeShape shape3({1, 32, -1, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    GeTensorDesc out_desc3(shape3, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape3);
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    read->AddOutputDesc(out_desc3);
    conv1->AddInputDesc(out_desc3);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);
    (void)ge::AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(conv2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(read, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);

    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr read_node = graph->AddNode(read);
    NodePtr concat_node = graph->AddNode(concat);
    /*
    *StridedRead
     *    \
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=1)
     *          |
     */
    ge::GraphUtils::AddEdge(read_node->GetOutDataAnchor(0),
                            conv1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  void InitGraph8(ComputeGraphPtr& graph) {
    OpDescPtr conv1 = std::make_shared<OpDesc>("conv1", CONV2D);
    OpDescPtr conv2 = std::make_shared<OpDesc>("conv2", CONV2D);
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 32, 64, 64});
    ge::GeShape shape2({1, 32, 64, 64});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    (void)ge::AttrUtils::SetInt(conv1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(conv2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetBool(conv1, ge::ATTR_NAME_CONTINUOUS_INPUT, true);
  
    conv1->AddOutputDesc(out_desc1);
    conv2->AddOutputDesc(out_desc2);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);

    // create nodes
    NodePtr conv1_node = graph->AddNode(conv1);
    NodePtr conv2_node = graph->AddNode(conv2);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *  Conv2d     Conv2d
     *      \       /
     *        Concat(concat_dim=1)
     *          |
     */
    ge::GraphUtils::AddEdge(conv1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
  }

  void InitGraph9(ComputeGraphPtr& graph) {
    OpDescPtr opdesc_ptr = std::make_shared<OpDesc>("test", "ReduceAllD");
    OpDescPtr concat = std::make_shared<OpDesc>("concat", CONCATD);
    OpDescPtr reshape1 = std::make_shared<OpDesc>("reshape1", RESHAPE);
    OpDescPtr reshape2 = std::make_shared<OpDesc>("reshape2", RESHAPE);

    (void)ge::AttrUtils::SetInt(concat, CONCAT_DIM, 1);

    // add descriptor
    ge::GeShape shape1({1, 32, 64, 64});
    ge::GeShape shape2({1, 32, 64, 64});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(reshape1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(reshape2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);


    opdesc_ptr->AddOutputDesc(out_desc1);
    reshape1->AddOutputDesc(out_desc1);
    reshape2->AddOutputDesc(out_desc1);
    reshape1->AddInputDesc(out_desc1);
    reshape2->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc1);
    concat->AddInputDesc(out_desc2);

    // create nodes
    NodePtr test_node = graph->AddNode(opdesc_ptr);
    NodePtr reshape1_node = graph->AddNode(reshape1);
    NodePtr reshape2_node = graph->AddNode(reshape2);
    NodePtr concat_node = graph->AddNode(concat);
    /*
     *         node
     *      /        \
     *  Reshape    Reshape
     *      \       /
     *        Concat(concat_dim=1)
     *          |
     */
    ge::GraphUtils::AddEdge(reshape1_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(reshape2_node->GetOutDataAnchor(0),
                            concat_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(test_node->GetOutDataAnchor(0),
                            reshape1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(test_node->GetOutDataAnchor(0),
                            reshape2_node->GetInDataAnchor(0));
  }

  void InitGraph10(ComputeGraphPtr& graph) {
    OpDescPtr conv = std::make_shared<OpDesc>("conv", CONV2D);
    OpDescPtr op_a = std::make_shared<OpDesc>("a", "A");
    OpDescPtr op_b = std::make_shared<OpDesc>("b", "B");

    OpDescPtr concat_1= std::make_shared<OpDesc>("concat_1", CONCATD);
    OpDescPtr concat_2= std::make_shared<OpDesc>("concat_2", CONCATD);
    OpDescPtr concat_3= std::make_shared<OpDesc>("concat_3", CONCATD);
    (void)ge::AttrUtils::SetInt(concat_1, CONCAT_DIM, 1);
    (void)ge::AttrUtils::SetInt(concat_2, CONCAT_DIM, 1);
    (void)ge::AttrUtils::SetInt(concat_3, CONCAT_DIM, 1);
    // add descriptor
    ge::GeShape shape1({1, 32, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    GeTensorDesc in_desc(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    in_desc.SetOriginFormat(ge::FORMAT_NCHW);
    in_desc.SetOriginDataType(ge::DT_FLOAT);
    in_desc.SetOriginShape(shape2);
    (void)ge::AttrUtils::SetInt(conv, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(op_a, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(op_b, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat_1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat_2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat_3, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);

    conv->AddOutputDesc(out_desc1);
    op_a->AddOutputDesc(out_desc2);
    op_b->AddOutputDesc(out_desc1);
    concat_1->AddInputDesc(in_desc);
    concat_1->AddInputDesc(in_desc);
    concat_1->AddOutputDesc(out_desc1);
    concat_2->AddInputDesc(in_desc);
    concat_2->AddInputDesc(in_desc);
    concat_2->AddOutputDesc(out_desc2);
    concat_3->AddInputDesc(in_desc);
    concat_3->AddInputDesc(in_desc);
    concat_3->AddOutputDesc(out_desc2);

    // create nodes
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr a_node = graph->AddNode(op_a);
    NodePtr b_node = graph->AddNode(op_b);
    NodePtr concat1_node = graph->AddNode(concat_1);
    NodePtr concat2_node = graph->AddNode(concat_2);
    NodePtr concat3_node = graph->AddNode(concat_3);
    /*
     *  A      Conv2d      B
     *   \       / \      /
     *    \     /   \    /
     *     Concat   Concat(concat_dim=1)
     *        \        /
     *         \      /
     *          Concat
     */
    ge::GraphUtils::AddEdge(a_node->GetOutDataAnchor(0),
                            concat1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                            concat1_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                            concat2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(b_node->GetOutDataAnchor(0),
                            concat2_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(concat1_node->GetOutDataAnchor(0),
                            concat3_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(concat2_node->GetOutDataAnchor(0),
                            concat3_node->GetInDataAnchor(1));
  }

  void InitGraph11(ComputeGraphPtr& graph) {
    OpDescPtr conv = std::make_shared<OpDesc>("conv", CONV2D);
    OpDescPtr op_a = std::make_shared<OpDesc>("a", "A");
    OpDescPtr op_b = std::make_shared<OpDesc>("b", "B");

    OpDescPtr concat_1= std::make_shared<OpDesc>("concat_1", CONCATD);
    OpDescPtr concat_2= std::make_shared<OpDesc>("concat_2", CONCATD);
    OpDescPtr concat_3= std::make_shared<OpDesc>("concat_3", CONCATD);
    (void)ge::AttrUtils::SetInt(concat_1, CONCAT_DIM, 1);
    (void)ge::AttrUtils::SetInt(concat_2, CONCAT_DIM, 1);
    (void)ge::AttrUtils::SetInt(concat_3, CONCAT_DIM, 1);
    // add descriptor
    ge::GeShape shape1({1, 32, 14, 14});
    ge::GeShape shape2({1, 32, 14, 14});
    GeTensorDesc out_desc1(shape1, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc1.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc1.SetOriginDataType(ge::DT_FLOAT);
    out_desc1.SetOriginShape(shape1);
    GeTensorDesc out_desc2(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    out_desc2.SetOriginFormat(ge::FORMAT_NCHW);
    out_desc2.SetOriginDataType(ge::DT_FLOAT);
    out_desc2.SetOriginShape(shape2);
    GeTensorDesc in_desc(shape2, ge::FORMAT_NCHW, ge::DT_FLOAT);
    in_desc.SetOriginFormat(ge::FORMAT_NCHW);
    in_desc.SetOriginDataType(ge::DT_FLOAT);
    in_desc.SetOriginShape(shape2);
    (void)ge::AttrUtils::SetInt(conv, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(op_a, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(op_b, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat_1, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat_2, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);
    (void)ge::AttrUtils::SetInt(concat_3, FE_IMPLY_TYPE, EN_IMPL_HW_TBE);

    conv->AddOutputDesc(out_desc1);
    op_a->AddOutputDesc(out_desc2);
    op_b->AddOutputDesc(out_desc1);
    concat_1->AddInputDesc(in_desc);
    concat_1->AddInputDesc(in_desc);
    concat_1->AddOutputDesc(out_desc1);
    concat_2->AddInputDesc(in_desc);
    concat_2->AddInputDesc(in_desc);
    concat_2->AddOutputDesc(out_desc2);
    concat_3->AddInputDesc(in_desc);
    concat_3->AddInputDesc(in_desc);
    concat_3->AddOutputDesc(out_desc2);

    // create nodes
    NodePtr conv_node = graph->AddNode(conv);
    NodePtr a_node = graph->AddNode(op_a);
    NodePtr b_node = graph->AddNode(op_b);
    NodePtr concat1_node = graph->AddNode(concat_1);
    NodePtr concat2_node = graph->AddNode(concat_2);
    NodePtr concat3_node = graph->AddNode(concat_3);
    /*
     *  A      Conv2d       B
     *   \       / \       /
     *    \     /   \ (1) /(0)
     *     Concat   Concat(concat_dim=1)
     *        \        /
     *         \      /
     *          Concat
     */
    ge::GraphUtils::AddEdge(a_node->GetOutDataAnchor(0),
                            concat1_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                            concat1_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(conv_node->GetOutDataAnchor(0),
                            concat2_node->GetInDataAnchor(1));
    ge::GraphUtils::AddEdge(b_node->GetOutDataAnchor(0),
                            concat2_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(concat1_node->GetOutDataAnchor(0),
                            concat3_node->GetInDataAnchor(0));
    ge::GraphUtils::AddEdge(concat2_node->GetOutDataAnchor(0),
                            concat3_node->GetInDataAnchor(1));
  }
};

TEST_F(UTEST_concat_c_to_n_optimize, checker_fail_for_unknownshape) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph1(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);
  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, MeetAlignmentConditionFromNCHWTo5HD_suc) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(2, 16);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, FORMAT_NCHW, DT_FLOAT16);
  out_desc.SetOriginShape(shape);
  op_desc->AddInputDesc(out_desc);

  ConcatCToNOptimizer concat_c_to_optimize;
  bool ret = concat_c_to_optimize.MeetAlignmentConditionFromNCHWTo5HD(op_desc);
  EXPECT_EQ(ret, true);
}

TEST_F(UTEST_concat_c_to_n_optimize, MeetAlignmentConditionFromNCHWTo5HD_suc1) {
  OpDescPtr op_desc = std::make_shared<OpDesc>("add", "Add");
  vector<int64_t> dim(2, 64);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, FORMAT_NCHW, ge::DT_INT4);
  out_desc.SetOriginShape(shape);
  op_desc->AddInputDesc(out_desc);

  ConcatCToNOptimizer concat_c_to_optimize;
  bool ret = concat_c_to_optimize.MeetAlignmentConditionFromNCHWTo5HD(op_desc);
  EXPECT_EQ(ret, true);
}

TEST_F(UTEST_concat_c_to_n_optimize, CheckPreNodeValid_suc) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = std::make_shared<OpDesc>("add", "Add");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  vector<int64_t> dim(1, 16);
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, FORMAT_NCHW, DT_FLOAT16);
  out_desc.SetOriginShape(shape);
  op_desc->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node = graph->AddNode(op_desc);
  NodePtr node_a = graph->AddNode(op_desc_a);
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node->GetInDataAnchor(0));

  ConcatCToNOptimizer concat_c_to_optimize;
  bool ret = concat_c_to_optimize.CheckPreNodeValid(node, node->GetName().c_str());
  EXPECT_EQ(ret, true);
}

TEST_F(UTEST_concat_c_to_n_optimize, IsDynamicGraph_suc) {
  ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  OpDescPtr op_desc = std::make_shared<OpDesc>("concat", "ConcatD");
  OpDescPtr op_desc_a = std::make_shared<OpDesc>("A", "AscendQuant");
  vector<int64_t> dim {1, -1, 16};
  GeShape shape(dim);
  GeTensorDesc out_desc(shape, FORMAT_NCHW, DT_FLOAT16);
  out_desc.SetOriginShape(shape);
  op_desc->AddInputDesc(out_desc);
  op_desc_a->AddOutputDesc(out_desc);

  NodePtr node = graph->AddNode(op_desc);
  NodePtr node_a = graph->AddNode(op_desc_a);
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node->GetInDataAnchor(0));
  GraphUtils::AddEdge(node_a->GetOutDataAnchor(0), node->GetInDataAnchor(1));


  ConcatCToNOptimizer concat_c_to_n_optimize;
  bool ret = concat_c_to_n_optimize.IsDynamicGraph(*graph);
  EXPECT_EQ(ret, true);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_001) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph3(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);

  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, true);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_002) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph4(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);

  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_003) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph5(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);
  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_004) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph6(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);
  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_005) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph7(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);
  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_006) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph8(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);
  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_007) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph9(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);
  auto concat_node = graph->FindNode("concat");
  bool attr_notask = false;
  (void)ge::AttrUtils::GetBool(concat_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask);
  EXPECT_EQ(attr_notask, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_008) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph10(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);

  auto concat1_node = graph->FindNode("concat_1");
  auto concat2_node = graph->FindNode("concat_2");
  auto concat3_node = graph->FindNode("concat_3");
  bool attr_notask_1 = false;
  bool attr_notask_2 = false;
  bool attr_notask_3 = false;

  (void)ge::AttrUtils::GetBool(concat1_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask_1);
  EXPECT_EQ(attr_notask_1, true);
  (void)ge::AttrUtils::GetBool(concat2_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask_2);
  EXPECT_EQ(attr_notask_2, false);
  (void)ge::AttrUtils::GetBool(concat3_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask_3);
  EXPECT_EQ(attr_notask_3, false);
}

TEST_F(UTEST_concat_c_to_n_optimize, concat_c_to_n_no_task_009) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>(GRAPH_NAME);
  InitGraph11(graph);
  ConcatCToNOptimizer concat_c_to_optimize;
  concat_c_to_optimize.SetFusionVirtualOp(*graph);

  auto concat1_node = graph->FindNode("concat_1");
  auto concat2_node = graph->FindNode("concat_2");
  auto concat3_node = graph->FindNode("concat_3");
  bool attr_notask_1 = false;
  bool attr_notask_2 = false;
  bool attr_notask_3 = false;

  (void)ge::AttrUtils::GetBool(concat1_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask_1);
  EXPECT_EQ(attr_notask_1, true);
  (void)ge::AttrUtils::GetBool(concat2_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask_2);
  EXPECT_EQ(attr_notask_2, false);
  (void)ge::AttrUtils::GetBool(concat3_node->GetOpDesc(), ATTR_NAME_NOTASK, attr_notask_3);
  EXPECT_EQ(attr_notask_3, false);
}
}