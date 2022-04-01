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

#ifndef GE_GRAPH_BUILD_GRAPH_BUILD_H_
#define GE_GRAPH_BUILD_GRAPH_BUILD_H_
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "framework/common/debug/log.h"
#include "common/debug/memory_dumper.h"
#include "common/properties_manager.h"
#include "framework/common/string_util.h"
#include "framework/common/types.h"
#include "framework/common/util.h"
#include "graph/build/model_builder.h"
#include "graph/build/task_generator.h"
#include "graph/compute_graph.h"
#include "external/graph/graph.h"
#include "graph/manager/graph_manager_utils.h"
#include "graph/model.h"
#include "graph/node.h"
#include "graph/partition/graph_partition.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/model/ge_root_model.h"

namespace ge {
class GraphBuilder {
 public:
  GraphBuilder();
  GraphBuilder(const GraphBuilder &in) = delete;
  GraphBuilder &operator=(const GraphBuilder &in) = delete;
  virtual ~GraphBuilder() = default;
  Status Build(ComputeGraphPtr &comp_graph, GeRootModelPtr &ge_root_model_ptr,
               uint64_t session_id = INVALID_SESSION_ID);
  void SetOptions(const GraphManagerOptions &options);
  static bool IsCompoundGraph(const ComputeGraphPtr &graph);

 private:
  Status CalcOpParam(const ge::ComputeGraphPtr &graph);
  Status GetTaskInfo(const ge::ModelBuilder &builder, const ModelPtr &model_ptr, ComputeGraphPtr &comp_graph,
                     Graph2SubGraphInfoList &subgraph_map, uint64_t session_id = INVALID_SESSION_ID) const;
  Status SetInputSize(const ge::NodePtr &node_ptr);
  Status UpdateDataInputSize(const ge::NodePtr &node_ptr) const;
  Status UpdateParentNodeOutputSize(const ge::ComputeGraphPtr &graph, const ge::NodePtr &parent_node_ptr) const;
  Status CalcDynShapeRootGraphDataSize(const ge::OpDescPtr &op_desc) const;
  Status SecondPartition(const ge::ComputeGraphPtr &comp_graph);
  Status MarkFpBpProfilingTaskAttr(ComputeGraphPtr &com_graph) const;
  Status BuildForDynamicShapeGraph(ComputeGraphPtr &comp_graph,
                                   GeRootModelPtr &ge_root_model_ptr, GeModelPtr &ge_model_ptr,
                                   uint64_t session_id = INVALID_SESSION_ID);
  Status BuildForKnownShapeGraph(ComputeGraphPtr &comp_graph,
                                 GeModelPtr &ge_model_ptr, uint64_t session_id = INVALID_SESSION_ID);
  Status BuildForUnknownShapeGraph(ComputeGraphPtr &comp_graph, GeModelPtr &ge_model_ptr,
                                   uint64_t session_id = INVALID_SESSION_ID);
  Status SetConstantInputOffset(const ComputeGraphPtr &comp_graph) const;
  Status AddOutputMemTypeForNode(const NodePtr &node) const;
  Status BuildForHostCpuGraph(ComputeGraphPtr &comp_graph, GeModelPtr &ge_model_ptr,
                              uint64_t session_id = INVALID_SESSION_ID);
  Status BuildForCompoundGraph(uint64_t session_id, const ComputeGraphPtr &root_graph,
                               const GeRootModelPtr &root_model);
  static Status DivideSubgraphFromRootGraph(const ComputeGraphPtr &root_graph, const ComputeGraphPtr &sub_root_graph);

  int32_t build_mode_;

  std::map<std::string, int32_t> stream_max_parallel_num_;
  bool hcom_parallel_;

  GraphPartitioner graph_partitioner_;
};
}  // namespace ge
#endif  // GE_GRAPH_BUILD_GRAPH_BUILD_H_
