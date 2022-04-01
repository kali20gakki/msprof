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

#ifndef GE_GRAPH_BUILD_MEMORY_GRAPH_MEM_ASSIGNER_H_
#define GE_GRAPH_BUILD_MEMORY_GRAPH_MEM_ASSIGNER_H_

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "framework/common/ge_inner_error_codes.h"
#include "graph/node.h"
#include "runtime/mem.h"
#include "graph/build/memory/hybrid_mem_assigner.h"

namespace ge {
struct MemoryOffset {
  MemoryOffset(rtMemType_t mem_type, size_t mem_offset) : mem_type_(mem_type), mem_offset_(mem_offset) {}

 public:
  rtMemType_t mem_type_;
  size_t mem_offset_;
};

using MemoryOffsetMap = std::map<int64_t, MemoryOffset>;

class VariableMemoryAssigner {
 public:
  explicit VariableMemoryAssigner(ComputeGraphPtr compute_graph) : compute_graph_(std::move(compute_graph)) {}

  VariableMemoryAssigner(const VariableMemoryAssigner &) = delete;

  VariableMemoryAssigner &operator=(const VariableMemoryAssigner &) = delete;

  virtual ~VariableMemoryAssigner() = default;

  ///
  /// @ingroup ge_graph
  /// @brief assign memory offset
  /// @return Status result of function
  ///
  Status Assign();

  ///
  /// @ingroup ge_graph
  /// @brief assign variable attr to nodes
  /// @return Status result of function
  ///
  Status AssignVarAttr2Nodes();

  Status AssignMemory2HasRefAttrNode();

 private:
  ComputeGraphPtr compute_graph_;
};

using VariableMemoryAssignerPtr = std::shared_ptr<VariableMemoryAssigner>;
using BlockMemAssignerPtr = std::shared_ptr<BlockMemAssigner>;
using HybridMemAssignerPtr = std::shared_ptr<HybridMemAssigner>;


class GraphMemoryAssigner {
 public:
  explicit GraphMemoryAssigner(ComputeGraphPtr compute_graph)
      : compute_graph_(std::move(compute_graph)),
        mem_assigner_(nullptr) {}

  GraphMemoryAssigner(const GraphMemoryAssigner &) = delete;

  GraphMemoryAssigner &operator=(const GraphMemoryAssigner &) = delete;

  virtual ~GraphMemoryAssigner() = default;

  ///
  /// @ingroup ge_graph
  /// @brief assign memory offset
  /// @return Status result of function
  ///
  Status AssignMemory();

  ///
  /// @ingroup ge_graph
  /// @brief assign variable attr to nodes,
  /// must be called after all memory assigned.
  /// @return Status result of function
  ///
  Status AssignVarAttr2Nodes();

  ge::Status AssignMemory2HasRefAttrNode() const;

  ge::Status ReAssignMemory(std::map<uint64_t, size_t> &mem_type_to_offset);

  Status AssignZeroCopyMemory(std::map<uint64_t, size_t> &mem_offset, size_t &zero_mem_copy_size);

  Status SetMemReuseInfo() const;

  void RecordSubsequentReuseNodeInfo(const int64_t op_index, const MemoryBlock *const memory_block,
                                     std::vector<MemReuseInfo> &mem_resue_info, uint32_t recursive_depth = 1U) const;

  Status SetInputOffset() const;

  Status UpdateOpInputOffset(const NodePtr &node) const;
  Status UpdateRefOpOutputOffset(const NodePtr &node, const std::map<int32_t, int32_t> &out2ins, const int32_t ref_in,
                                 const int64_t input_offset) const;

  Status CheckOffset() const;
  Status CheckRefNodeOffset(const NodePtr &node) const;

  Status AssignReferenceMemory() const;

  void MarkDistanceAttr();

  void MarkNodeDistanceAttr(const NodePtr &node,
                            std::map<size_t, std::pair<NodePtr, std::vector<int64_t>>> &mem_block_visit_info,
                            const std::map<std::string, int64_t> &node_index_in_stream);

 private:
  ///
  /// @ingroup ge_graph
  /// @brief assign memory offset
  /// @return Status result of function
  ///
  Status ReAssignContinuousMemory();

  Status ReAssignAtomicMemory();

  Status TryGetNodeRefIndexes(const NodePtr &node, std::map<int32_t, int32_t> &out2ins) const;

  bool AssignContinuousInputMemoryWithAtomicProcessDirectly(const NodePtr &input_continuous_node,
                                                            std::map<NodePtr, uint32_t> &node_2_continuous_type) const;

  Status AssignContinuousInputMemoryWithAtomicProcess(const NodePtr &input_continuous_node,
                                                      uint32_t continuous_type, bool reverse_refresh = false);

  Status FilterAtomicNodesForMemoryAssign(std::map<std::string, std::map<NodePtr, std::vector<NodePtr>>> &atomic_nodes,
                                          std::map<std::string, std::vector<NodePtr>> &connecting_output_atomic_nodes);

  Status SetMemOffset(const NodePtr &node, const InDataAnchorPtr &in_data_anchor, bool reverse_refresh,
                      int64_t &mem_offset, int64_t &continuous_mem_start) const;

  Status AssignContinuousInputMemory(const NodePtr &node, int64_t &continuous_mem_start,
                                     int64_t &continuous_mem_size, int64_t memory_type, uint32_t continuous_type,
                                     bool reverse_refresh = false);

  Status AssignContinuousOutputMemory(const NodePtr &node, int64_t memory_type, uint32_t continuous_type) const;

  ///
  /// @brief check the input of node whether support atomic attr
  /// @param node
  /// @return true:supported; false:not supported
  ///
  bool CheckInputIsSupportAtomic(const NodePtr &node) const;

  bool CheckAtomicNodeIsSupportRef(const OpDescPtr &op_desc) const;

  Status GetMemoryAssignmentStatus(const NodePtr &node, int64_t output_index, bool &is_mem_assigned) const;

  Status AssignAtomicOutputMemory(const NodePtr &node, std::vector<int64_t> &mem_offset_end);

  Status AssignOrdinaryAtomicWorkspaceMemory(const OpDescPtr &op_desc,
                                             std::map<std::string, std::map<int64_t, int64_t>> &workspace_info,
                                             std::vector<int64_t> &mem_offset_end);

  Status AssignFusionAtomicWorkspaceMemory(const OpDescPtr &op_desc,
                                           std::map<std::string, std::map<int64_t, int64_t>> &workspace_info,
                                           std::vector<int64_t> &mem_offset_end);

  Status AssignAtomicOutputAndWorkspaceMemory(const NodePtr &node, std::vector<int64_t> &mem_offset_end);

  Status AssignConnectNetOutputAtomicMemory(std::vector<NodePtr> &connect_netoutput_nodes);

  Status SetIndependentAtomicAttr(const NodePtr &node, int64_t atomic_mem_start,
                                  const std::vector<int64_t> &mem_offset_end, int64_t memory_type) const;

  Status SetAtomicCleanAttr(const NodePtr &node, const std::vector<int64_t> &atomic_mem_start,
                            const std::vector<int64_t> &atomic_mem_size, int64_t memory_type) const;

  Status IsIndependentAtomicClean(const NodePtr &node, bool &is_independent_atomic_clean_node);

  void AlignMemOffset(const int64_t &mem_align_size, int64_t memory_type);

  Status UpdateOpInputOffset(const NodePtr &node, std::vector<int64_t> &input_list) const;

  Status UpdateConstArgsOffset(const NodePtr &node, std::vector<int64_t> &input_list) const;

  Status UpdateOpInputDescOffset(const NodePtr &node) const;

  NodePtr GetKnownInputNode(const NodePtr &node) const;

  Status GetNodeMemoryType(const NodePtr &node, int64_t &memory_type, std::string input_or_output) const;
  Status GetNodeListMemoryType(const std::vector<NodePtr> &nodes, int32_t mem_reuse_model,
                               int64_t &memory_type) const;

  bool CheckContinuousMemType(std::vector<int64_t> mem_type_list) const;

  void PrintMemoryOffset() const;

  Status AssignBufferPoolMemory();

  bool IsRefFromInputOpCascade(const NodePtr &node) const;

  Status UpdateRefOpOffsetReverse(const NodePtr &node) const;

  bool IsOutputVisitedByMultiStream(const NodePtr &peer_out_node, int64_t out_anchor_index) const;

  void UpdatePrevNodeInputDesc(const NodePtr &prev_node,
                               const std::vector<int64_t> &prev_node_input_index_vec,
                               int64_t distance) const;

  void UpdateCurNodeInputDesc(const NodePtr &cur_node, int64_t cur_node_input_index, int64_t distance) const;

  void CheckNeedCalcDistAndUpdateVisitInfo(const NodePtr &peer_out_node,
                                           const OutDataAnchorPtr &peer_out_anchor,
                                           size_t matched_mem_offset,
                                           std::map<size_t, std::pair<NodePtr, std::vector<int64_t>>> &mem_block_visit_info,
                                           bool &is_need_calc_distance) const;

  void CalcDistanceAndUpdateDesc(const std::map<std::string, int64_t> &node_index_in_stream,
                                 const InDataAnchorPtr &in_data_anchor,
                                 size_t matched_mem_offset,
                                 const NodePtr &node,
                                 std::map<size_t, std::pair<NodePtr, std::vector<int64_t>>> &mem_block_visit_info,
                                 bool &is_need_skip) const;

  void DeleteVisitInfoWhenLifecycleEnded(const NodePtr &node,
                                         const InDataAnchorPtr &in_data_anchor,
                                         size_t matched_mem_offset,
                                         std::map<size_t, std::pair<NodePtr,
                                         std::vector<int64_t>>> &mem_block_visit_info) const;

  MemoryOffsetMap memory_offset_;
  ComputeGraphPtr compute_graph_;
  HybridMemAssignerPtr mem_assigner_;
};
}  // namespace ge

#endif  // GE_GRAPH_BUILD_MEMORY_GRAPH_MEM_ASSIGNER_H_
