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

#include "graph_builder_utils.h"
#include "graph/utils/graph_utils.h"
#include <set>

namespace fe {
namespace ut {
ge::NodePtr ComputeGraphBuilder::AddNode(const std::string &name,
                                         const std::string &type, int in_cnt,
                                         int out_cnt, ge::Format format,
                                         ge::DataType data_type,
                                         std::vector<int64_t> shape) {
  auto tensor_desc = std::make_shared<ge::GeTensorDesc>();
  tensor_desc->SetShape(ge::GeShape(std::move(shape)));
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);

  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  for (int i = 0; i < in_cnt; ++i) {
    op_desc->AddInputDesc(tensor_desc->Clone());
  }
  for (int i = 0; i < out_cnt; ++i) {
    op_desc->AddOutputDesc(tensor_desc->Clone());
  }

  return graph_->AddNode(op_desc);
}

ge::NodePtr ComputeGraphBuilder::AddNodeWithImplyType(const std::string &name,
                                         const std::string &type, int in_cnt,
                                         int out_cnt, ge::Format format,
                                         ge::DataType data_type,
                                         std::vector<int64_t> shape) {
  auto tensor_desc = std::make_shared<ge::GeTensorDesc>();
  tensor_desc->SetShape(ge::GeShape(std::move(shape)));
  tensor_desc->SetFormat(format);
  tensor_desc->SetDataType(data_type);

  auto op_desc = std::make_shared<ge::OpDesc>(name, type);
  for (int i = 0; i < in_cnt; ++i) {
    op_desc->AddInputDesc(tensor_desc->Clone());
  }
  for (int i = 0; i < out_cnt; ++i) {
    op_desc->AddOutputDesc(tensor_desc->Clone());
  }
  ge::AttrUtils::SetInt(op_desc, "_fe_imply_type", 6);
  return graph_->AddNode(op_desc);
}

void ComputeGraphBuilder::AddDataEdge(ge::NodePtr &src_node, int src_idx,
                                      ge::NodePtr &dst_node, int dst_idx) {
  ge::GraphUtils::AddEdge(src_node->GetOutDataAnchor(src_idx),
                          dst_node->GetInDataAnchor(dst_idx));
}
void ComputeGraphBuilder::AddControlEdge(ge::NodePtr &src_node,
                                         ge::NodePtr &dst_node) {
  ge::GraphUtils::AddEdge(src_node->GetOutControlAnchor(),
                          dst_node->GetInControlAnchor());
}

std::set<std::string> GetNames(const ge::Node::Vistor<ge::NodePtr> &nodes) {
  std::set<std::string> names;
  for (auto &node : nodes) {
    names.insert(node->GetName());
  }
  return names;
}

std::set<std::string>
GetNames(const ge::ComputeGraph::Vistor<ge::NodePtr> &nodes) {
  std::set<std::string> names;
  for (auto &node : nodes) {
    names.insert(node->GetName());
  }
  return names;
}
} // namespace ut
} // namespace fe