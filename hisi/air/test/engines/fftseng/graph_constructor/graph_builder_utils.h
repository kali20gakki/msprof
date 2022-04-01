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

#ifndef LLT_FUSION_ENGINE_GRAPH_CONSTRUCTOR_GRAPH_BUILDER_UTILS_H_
#define LLT_FUSION_ENGINE_GRAPH_CONSTRUCTOR_GRAPH_BUILDER_UTILS_H_

#include <vector>
#include <string>
#include <set>

#include "graph/node.h"
#include "graph/graph.h"
#include "graph/compute_graph.h"

namespace ffts {
namespace ut {
class ComputeGraphBuilder {
 public:
  explicit ComputeGraphBuilder(const std::string &name) {
    graph_ = std::make_shared<ge::ComputeGraph>(name);
  }
  ge::NodePtr AddNode(const std::string &name,
                  const std::string &type,
                  int in_cnt,
                  int out_cnt,
                  ge::Format format = ge::FORMAT_NCHW,
                  ge::DataType data_type = ge::DT_FLOAT,
                  std::vector<int64_t> shape = {1, 1, 224, 224});
  ge::NodePtr AddNodeWithImplyType(const std::string &name,
                      const std::string &type,
                      int in_cnt,
                      int out_cnt,
                      ge::Format format = ge::FORMAT_NCHW,
                      ge::DataType data_type = ge::DT_FLOAT,
                      std::vector<int64_t> shape = {1, 1, 224, 224});
  void AddDataEdge(ge::NodePtr &src_node, int src_idx, ge::NodePtr &dst_node, int dst_idx);
  void AddControlEdge(ge::NodePtr &src_node, ge::NodePtr &dst_node);
  ge::ComputeGraphPtr GetGraph() {
    graph_->TopologicalSorting();
    return graph_;
  }
 private:
  ge::ComputeGraphPtr graph_;
};

std::set<std::string> GetNames(const ge::Node::Vistor<ge::NodePtr> &nodes);
std::set<std::string> GetNames(const ge::ComputeGraph::Vistor<ge::NodePtr> &nodes);
}
}

#endif  // LLT_FUSION_ENGINE_GRAPH_CONSTRUCTOR_GRAPH_BUILDER_UTILS_H_
