/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

// To test the RefIdentityDeleteOpPass

#include <string>
#include <gtest/gtest.h>
#include "graph/passes/ref_identity_delete_op_pass.h"

using namespace domi;
using namespace ge;
class UTestRefIdentityDeleteOpPass : public testing::Test {
 protected:
  void SetUp() {}
  void TearDown() {}
};

class NodeBuilder {
 public:
  NodeBuilder(const std::string& name, const std::string& type) {
    op_desc_ = std::make_shared<OpDesc>(name, type);
  }

  NodeBuilder& AddInputDesc(const std::string &name, const std::vector<int64_t> &shape,
                            ge::Format format = FORMAT_NCHW,
                            ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddInputDesc(name, ge::GeTensorDesc(GeShape(shape), format, data_type));
    return *this;
  }

  NodeBuilder& AddOutputDesc(const std::string &name, const std::vector<int64_t> &shape,
                             ge::Format format = FORMAT_NCHW,
                             ge::DataType data_type = DT_FLOAT) {
    op_desc_->AddOutputDesc(name, ge::GeTensorDesc(GeShape(shape), format, data_type));
    return *this;
  }

  ge::NodePtr Build(const ge::ComputeGraphPtr& graph) {
    return graph->AddNode(op_desc_);
  }

 private:
  ge::OpDescPtr op_desc_;
};

///   variable
///      |
///      |
///  refidentity-->variable_ref   ==>     variable--------------
///      |          ^   |                     |                |
///      |          |   |                     |                |
/// applymomentum---|   |                 aplymomentum--->variable_ref
///      |              |                     |
///      |              |                     |
///     add<-------------                    add
TEST_F(UTestRefIdentityDeleteOpPass, ref_identity_delete_without_transnode_success) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::NodePtr variable_node = NodeBuilder("variable", VARIABLE)
          .AddInputDesc("input", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("output", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);

  ge::NodePtr ref_identity_node = NodeBuilder("RefIdentity", REFIDENTITY)
          .AddInputDesc("input", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("output", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);

  ge::NodePtr apply_monetum_node = NodeBuilder("Applymomentum", APPLYMOMENTUM)
          .AddInputDesc("var", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("no_var", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);

  ge::NodePtr add_node = NodeBuilder("Add", ADD)
          .AddInputDesc("x", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("y", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);
          
  ge::NodePtr variable_ref = NodeBuilder("VariableRef", VARIABLE)
          .AddInputDesc("x", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("y", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);

  ge::GraphUtils::AddEdge(variable_node->GetOutDataAnchor(0), ref_identity_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(ref_identity_node->GetOutDataAnchor(0), apply_monetum_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(ref_identity_node->GetOutDataAnchor(0), variable_ref->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(apply_monetum_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(variable_ref->GetOutControlAnchor(), add_node->GetInControlAnchor());
  ge::GraphUtils::AddEdge(apply_monetum_node->GetOutControlAnchor(), variable_ref->GetInControlAnchor());
  auto desc = apply_monetum_node->GetOpDesc()->GetInputDesc(0);
  desc.SetRefPortByIndex({0});
  apply_monetum_node->GetOpDesc()->UpdateInputDesc(0, desc);

  ge::RefIdentityDeleteOpPass pass_;
  ge::Status status = pass_.Run(graph);
  EXPECT_EQ(status, ge::SUCCESS);

  EXPECT_EQ(variable_node->GetOutDataNodes().size(), 2);
  EXPECT_EQ(variable_node->GetOutControlNodes().size(), 0);
  const auto name0 = variable_node->GetOutDataNodes().at(0)->GetName();
  const auto name1 = variable_node->GetOutDataNodes().at(1)->GetName();
  const bool check0 = (name0 == "VariableRef" || name1 == "VariableRef");
  const bool check1 = (name0 == "Applymomentum" || name1 == "Applymomentum");
  EXPECT_EQ(check0, true);
  EXPECT_EQ(check1, true);

  EXPECT_EQ(apply_monetum_node->GetOutDataNodes().size(), 1);
  EXPECT_EQ(apply_monetum_node->GetOutControlNodes().size(), 1);
  EXPECT_EQ(apply_monetum_node->GetOutDataNodes().at(0)->GetName(), "Add");
  EXPECT_EQ(apply_monetum_node->GetOutControlNodes().at(0)->GetName(), "VariableRef");

  EXPECT_EQ(variable_ref->GetOutControlNodes().size(), 1);
}

TEST_F(UTestRefIdentityDeleteOpPass, ref_identity_delete_without_ref_identity) {
  ge::ComputeGraphPtr graph = std::make_shared<ComputeGraph>("test");
  ge::NodePtr variable_node = NodeBuilder("variable", VARIABLE)
          .AddInputDesc("input", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("output", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);

  ge::NodePtr apply_monetum_node = NodeBuilder("Applymomentum", APPLYMOMENTUM)
          .AddInputDesc("var", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("no_var", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);

  ge::NodePtr add_node = NodeBuilder("Add", ADD)
          .AddInputDesc("x", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("y", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);
          
  ge::NodePtr variable_ref = NodeBuilder("VariableRef", VARIABLE)
          .AddInputDesc("x", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .AddOutputDesc("y", {2, 16, 2, 2}, FORMAT_NHWC, DT_FLOAT)
          .Build(graph);

  ge::GraphUtils::AddEdge(variable_node->GetOutDataAnchor(0), apply_monetum_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(apply_monetum_node->GetOutDataAnchor(0), add_node->GetInDataAnchor(0));
  ge::GraphUtils::AddEdge(variable_ref->GetOutControlAnchor(), add_node->GetInControlAnchor());
  ge::GraphUtils::AddEdge(apply_monetum_node->GetOutControlAnchor(), variable_ref->GetInControlAnchor());
  auto desc = apply_monetum_node->GetOpDesc()->GetInputDesc(0);
  desc.SetRefPortByIndex({0});
  apply_monetum_node->GetOpDesc()->UpdateInputDesc(0, desc);

  ge::RefIdentityDeleteOpPass pass_;
  ge::Status status = pass_.Run(graph);
  EXPECT_EQ(status, ge::SUCCESS);
}