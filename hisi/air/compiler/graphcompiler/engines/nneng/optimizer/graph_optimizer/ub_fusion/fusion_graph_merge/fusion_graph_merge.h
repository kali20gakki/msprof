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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_FUSION_GRAPH_MERGE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_FUSION_GRAPH_MERGE_H_

#include <map>
#include <string>
#include <vector>
#include "common/aicore_util_types.h"
#include "common/graph_comm.h"
#include "common/util/op_info_util.h"

namespace fe {
using ScopeNodeMap = std::map<int64_t, std::vector<ge::NodePtr>>;
using GraphCommPtr = std::shared_ptr<GraphComm>;

class FusionGraphMerge;
using FusionGraphMergePtr = std::shared_ptr<FusionGraphMerge>;

class FusionGraphMerge {
 public:
  FusionGraphMerge(const std::string &scope_attr, const GraphCommPtr &graph_comm_ptr);
  virtual ~FusionGraphMerge();
  FusionGraphMerge(const FusionGraphMerge &in) = delete;
  FusionGraphMerge &operator=(const FusionGraphMerge &in) = delete;

  Status MergeFusionGraph(ge::ComputeGraph &fusion_graph);

  const std::string& GetScopeAttr() const;

 private:
  Status MergeFusionNodes(ge::ComputeGraph &fusion_graph);
  Status GetScopeNodeMap(const ge::ComputeGraph &fusion_graph, ScopeNodeMap &fusion_scope_map) const;
  Status MergeEachFusionNode(ge::ComputeGraph &fusion_graph, std::vector<ge::NodePtr> &fus_nodelist);
  Status MergeFusionNodeL2Info(const ge::ComputeGraph &fusion_graph);

  virtual Status AfterMergeFusionGraph(ge::ComputeGraph &graph) {
    return SUCCESS;
  }

  void SetAtomicFlagAndOutputIndex(const ge::NodePtr &first_node, const ge::NodePtr &fus_node) const;
  Status SetL2TaskInfoToFusionOp(ge::NodePtr fus_node) const;

  Status UpdateL2Info(const int64_t &origin_index, const int64_t &fusion_index, const L2FusionInfoPtr &originl2_info,
                      const L2FusionInfoPtr &fusion_l2_info) const;

  Status SetL2NameAndIndex(const L2FusionInfoPtr &originl2_info, L2FusionInfoPtr &fusion_l2_info) const;

  Status CreateFusionOpNodeGraph(vector<FusionDataFlow> &fus_input_edge_list,
                                 vector<FusionDataFlow> &fus_output_edge_list, vector<ge::NodePtr> &fus_nodelist,
                                 ge::OpDescPtr fusion_op_desc, ge::ComputeGraph &orig_graph);

  Status AddFusionNodeOpDesc(ge::OpDescPtr &fus_op, vector<FusionDataFlow> &fus_input_edge_list,
                             vector<FusionDataFlow> &fus_output_edge_list);

  Status AddFusionNodeOutputDesc(ge::OpDescPtr fus_op, std::vector<FusionDataFlow> &fus_output_edge_list);

  Status AddFusionNodeInputDesc(ge::OpDescPtr fus_op, std::vector<FusionDataFlow> &fus_input_edge_list);

  void UpdateOutputRefPortIndex(ge::OpDescPtr &in_edge_dst_op_desc_ptr, const ge::OpDescPtr &fus_op,
                                const uint32_t dst_anchor_index) const;

  void SetMultiKernelOutPutOffsets(const ge::OpDescPtr &src_op, size_t src_out_idx, const ge::OpDescPtr &fus_op,
                                   std::vector<int64_t> &save_pre_output_offset) const;

  void UpdateOutputSgtSliceInfo(const ge::OpDescPtr &src_op, size_t src_out_idx, ge::OpDescPtr &fus_op,
                                std::vector<int64_t> &save_pre_output_offset) const;

  void UpdateL1Attr(ge::OpDescPtr op_desc_ptr, const string &attr_key, const uint32_t &anchor_index,
                    const uint32_t &tensor_desc_index, vector<int64_t> &target_vec) const;

  Status RefreshFusionNodeDataFlow(ge::NodePtr fus_node, const ge::ComputeGraph &fusion_graph);

  void AddBuffFusionNodeInputDesc(vector<int> &in_mem_type_old_node,
                                  ge::OpDescPtr &in_edge_dst_op_desc_ptr,
                                  const ge::DataAnchorPtr &in_edge_dst_data_anchor_ptr,
                                  vector<int64_t> &FusNodeInputOffset,
                                  vector<int> &in_mem_type_fus_node) const;

  Status SetDataOutPutMapingAttr(
      std::map<ge::NodePtr, std::map<ge::AnchorPtr, ge::AnchorPtr>> fusion_op_anchors_map) const;

  void SetDataDumpRef(ge::NodePtr fus_node, const ge::ComputeGraph &fusion_graph) const;

  void SetDataDumpRefForInDataAnchors(ge::NodePtr fus_node) const;

  void SetDataDumpRefForOutputDataAnchors(ge::NodePtr fus_node) const;

  Status SetL2NameAndIndexForUnfusNode(L2FusionInfoPtr &originl2_info);

  Status GetFusionAnchorInfo(const std::string &origin_name, std::map<std::int64_t, std::int64_t> &out_index_map,
                             ge::NodePtr &fusion_node) const;

  void CreateOriginalFusionOpGraph(ge::NodePtr &fus_node_ptr, vector<ge::NodePtr> &fus_nodelist) const;

  Status CalcStridedWriteOutSize(const ge::NodePtr &fus_node_ptr, vector<ge::NodePtr> &fus_nodelist) const;

  Status CalcStridedReadInSize(const ge::NodePtr &fus_node_ptr, vector<ge::NodePtr> &fus_nodelist) const;

  std::map<std::string, std::map<std::int64_t, ge::NodePtr>> fusion_op_name_map_all_;

  std::string scope_attr_;
  GraphCommPtr graph_comm_ptr_;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_UB_FUSION_FUSION_GRAPH_MERGE_H_
