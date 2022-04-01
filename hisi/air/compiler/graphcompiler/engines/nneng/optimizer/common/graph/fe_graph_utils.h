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

#ifndef FUSION_ENGINE_OPTIMIZER_COMMON_GRAPH_FE_GRAPH_UTILS_H_
#define FUSION_ENGINE_OPTIMIZER_COMMON_GRAPH_FE_GRAPH_UTILS_H_
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "common/fe_utils.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ref_relation.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/node_utils.h"
#include "graph/utils/type_utils.h"

namespace fe {
struct RelationUpdateInfo {
  ge::Format primary_format = ge::FORMAT_RESERVED;
  int32_t sub_format = 0;
  ge::GeShape shape;
  ge::DataType data_type = ge::DT_UNDEFINED;
  string attr_name;
  int attr_value = -1;

  RelationUpdateInfo(const ge::Format &update_primary_format, const int32_t &update_sub_format,
                     const ge::GeShape &update_shape, string update_attr_name, int update_attr_value) {
    primary_format = update_primary_format;
    sub_format = update_sub_format;
    shape = update_shape;
    attr_name = update_attr_name;
    attr_value = update_attr_value;
  };

  RelationUpdateInfo(const ge::DataType &update_data_type, string update_attr_name, int update_attr_value) {
    data_type = update_data_type;
    attr_name = update_attr_name;
    attr_value = update_attr_value;
  };
};

class FeGraphUtils {
 public:
  static void DumpSubGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix);

  static void DumpGraphAndOnnx(const ge::ComputeGraph &graph, const std::string &suffix);
  static void DumpGraph(const ge::ComputeGraph &graph, const std::string &suffix);

  static void IsNodeSpecificType(const std::unordered_set<string> &types, ge::NodePtr &node,
                                 bool &matched);

  static bool IsPeerOutConst(const ge::Node *node, const int &anchor_index, ge::NodePtr &peer_out_node);

  static bool IsPeerOutWeight(ge::Node *node, const int &anchor_index, ge::NodePtr &peer_out_node);
  
  static bool IsMainGraphData(const ge::OpDescPtr &op_desc_ptr);

  static bool IsMainGraphNetOutput(const ge::OpDescPtr &op_desc_ptr);

  static bool IsSubGraphData(const ge::OpDescPtr &op_desc_ptr);

  static bool IsSubGraphNetOutput(const ge::OpDescPtr &op_desc_ptr);

  static bool IsSubGraphDataOrNetOutput(const ge::OpDescPtr &op_desc_ptr);

  static bool IsNotSubGraphDataAndNetOutput(const ge::OpDescPtr &op_desc_ptr);

  static Status GetPreOutAnchorOfSubData(const ge::NodePtr &data_node_ptr,
                                         ge::OutDataAnchorPtr &pre_out_data_anchor_ptr);

  static Status GetPreSubNetoutputInAnchor(std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections,
                                           std::vector<ge::InDataAnchorPtr> &vec_netoutput_in_ahchor);

  static Status GetNextInAnchorsOfSubNetOutput(const ge::NodePtr &net_output_node_ptr, const int &input_index,
                                               std::vector<ge::InDataAnchorPtr> &next_in_data_anchors);

  static Status GetNextSubDatasOutAnchors(std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections,
                                          std::vector<ge::OutDataAnchorPtr> &out_data_anchors);

  static Status UpdateFormatOfRelatedEdges(const std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections,
                                           const RelationUpdateInfo &relation_update_info_a);

  static bool CheckRelatedEdgesOriginShape(const std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections);

  static Status CloneGraph(ge::ComputeGraph &origin_graph, ge::ComputeGraph &clone_graph);

  static Status AddEdge(ge::AnchorPtr src_anchor, ge::NodePtr dst_node, ge::AnchorPtr old_dst_anchor);

  static void GetGraphIdFromAttr(const ge::ComputeGraph &graph, string &graph_id);

  static bool CheckSubGraphDataForConstValue(ge::NodePtr &parent_node);
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_COMMON_GRAPH_FE_GRAPH_UTILS_H_
