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

#include <cstdint>
#include <iostream>
#include <string>

#define protected public
#define private public
#include "common/ge_inner_error_codes.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "ge/ge_api.h"
#include "ge_attr_value.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/passes/mark_same_addr_pass.h"
#include "graph/utils/op_desc_utils.h"
#include "inc/pass_manager.h"
#include "init/gelib.h"
#include "opskernel_manager/ops_kernel_manager.h"
#undef protected
#undef private

using namespace domi;

namespace ge {
namespace {
class MarkSameAddrPassTest : public testing::Test {
 public:
  void SetUp() {}

  void TearDown() {}

 protected:
  MarkSameAddrPassTest() {
    graph_ = std::make_shared<ComputeGraph>("test");
    vector<int64_t> shape_vec{1, 1, 224, 224};
    GeShape shape = GeShape(shape_vec);
    default_tensor_desc_ = std::make_shared<GeTensorDesc>();
    default_tensor_desc_->SetShape(shape);
    default_tensor_desc_->SetFormat(FORMAT_NCHW);
    default_tensor_desc_->SetDataType(DT_FLOAT);
    pass_manager_.AddPass("", new MarkSameAddrPass());
  }

  NodePtr NewNode(const std::string &name, const std::string &type, int input_cnt, int output_cnt) {
    OpDescPtr op_desc = std::make_shared<OpDesc>(name, type);
    for (int i = 0; i < input_cnt; ++i) {
      op_desc->AddInputDesc(default_tensor_desc_->Clone());
    }

    for (int i = 0; i < output_cnt; ++i) {
      op_desc->AddOutputDesc(default_tensor_desc_->Clone());
    }

    NodePtr node = graph_->AddNode(op_desc);
    if (type == CONSTANT) {
      int32_t weight[] = {1};
      GeTensorDesc weight_desc(GeShape({1}), FORMAT_NHWC, DT_INT32);
      GeTensorPtr tensor = std::make_shared<GeTensor>(weight_desc, (uint8_t *)weight, sizeof(weight));
      vector<GeTensorPtr> tensor_vec = {tensor};
      OpDescUtils::SetWeights(node, tensor_vec);
    }

    return node;
  }

  ComputeGraphPtr graph_;
  GeTensorDescPtr default_tensor_desc_;
  MarkSameAddrPass mark_same_addr_pass_;
  PassManager pass_manager_;
};
}  // namespace

TEST_F(MarkSameAddrPassTest, NullInput) {
  auto ret = mark_same_addr_pass_.Run(nullptr);
  EXPECT_EQ(ret, PARAM_INVALID);
}

TEST_F(MarkSameAddrPassTest, EmptyGraph) {
  auto ret = mark_same_addr_pass_.Run(graph_);
  EXPECT_EQ(ret, SUCCESS);
}

/*     Op1
 *      |
 *     Op2  --->  partitioncall
 *      |
 *  NetOutput
 */

TEST_F(MarkSameAddrPassTest, ParentNodeHasNoAttr) {
  auto node1 = NewNode("Op1", DATA_TYPE, 0, 1);
  auto node2 = NewNode("Op2", STREAMSWITCH, 1, 1);
  auto net_output = NewNode("NetOutput", NETOUTPUT, 3, 3);

  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node2->GetOutDataAnchor(0), net_output->GetInDataAnchor(1));

  auto parent_node = NewNode("partitioncall", PARTITIONEDCALL, 1, 1);
  graph_->SetParentNode(parent_node);

  Status ret = mark_same_addr_pass_.Run(graph_);
  EXPECT_EQ(ret, SUCCESS);
}

/*     Op1
 *      |
 *     Op2  --->  partitioncall(unknown)
 *      |
 *  NetOutput
 */
TEST_F(MarkSameAddrPassTest, ParentNodeHasUnknownAttr) {
  auto node1 = NewNode("Op1", DATA_TYPE, 0, 1);
  auto node2 = NewNode("Op2", STREAMSWITCH, 1, 1);
  OpDescPtr op_desc2 = node2->GetOpDesc();
  auto net_output = NewNode("NetOutput", NETOUTPUT, 3, 3);

  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node2->GetOutDataAnchor(0), net_output->GetInDataAnchor(1));
  auto parent_node = NewNode("partitioncall", PARTITIONEDCALL, 1, 1);
  AttrUtils::SetBool(parent_node->GetOpDesc(), ATTR_NAME_IS_UNKNOWN_SHAPE, true);
  graph_->SetParentNode(parent_node);
  graph_->SetGraphUnknownFlag(true);
  Status ret = mark_same_addr_pass_.Run(graph_);
  EXPECT_EQ(ret, SUCCESS);
}

//
///*     Op1
// *      |
// *     Op2  --->  partitioncall(known)
// *      |
// *  NetOutput
// */
TEST_F(MarkSameAddrPassTest, ParentNodeHasKnownAttr) {
  auto node1 = NewNode("Op1", DATA_TYPE, 0, 1);
  auto node2 = NewNode("Op2", STREAMSWITCH, 1, 1);
  OpDescPtr op_desc2 = node2->GetOpDesc();
  op_desc2->SetAttr("atomic_input_index", GeAttrValue::CreateFrom<GeAttrValue::LIST_INT>({123, 456}));
  auto net_output = NewNode("NetOutput", NETOUTPUT, 3, 3);

  GraphUtils::AddEdge(node1->GetOutDataAnchor(0), node2->GetInDataAnchor(0));
  GraphUtils::AddEdge(node2->GetOutDataAnchor(0), net_output->GetInDataAnchor(1));
  auto parent_node = NewNode("partitioncall", PARTITIONEDCALL, 1, 1);
  AttrUtils::SetBool(parent_node->GetOpDesc(), ATTR_NAME_IS_UNKNOWN_SHAPE, false);
  graph_->SetParentNode(parent_node);
  Status ret = mark_same_addr_pass_.Run(graph_);
  EXPECT_EQ(ret, SUCCESS);
}
}  // namespace ge