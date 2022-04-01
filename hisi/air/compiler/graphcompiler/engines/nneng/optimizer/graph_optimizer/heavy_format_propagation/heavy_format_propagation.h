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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_PROPAGATION_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_PROPAGATION_H_

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "adapter/common/op_store_adapter_manager.h"
#include "common/fe_inner_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/math_util.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "format_selector/manager/format_dtype_querier.h"
#include "graph/compute_graph.h"
#include "graph/ref_relation.h"
#include "graph_optimizer/heavy_format_propagation/heavy_format_selector.h"
#include "graph_optimizer/heavy_format_propagation/heavy_format_supportformats_updater.h"

namespace fe {
constexpr int32_t INVALID_FORMAT_INDEX = 0xffff;
constexpr int32_t INVALID_LAST_NODE_ANCHOR_INDEX = 0xffffffff;

enum class FormatSelectionType {
  /* FORMAT_DEPENDS_ON_OP_KERNEL_INFO means this op can not use arbitrary
   * format. So we need to loop up for the ops kernel first to get all supported
   * formats. */
  FORMAT_DEPENDS_ON_OP_KERNEL_INFO = 0,

  /* FORMAT_AGNOSTIC_FOR_ALL_INPUTS_AND_OUTPUTS means this op can use arbitrary
   * format. And we need to set the format of all inputs and outputs as the
   * same. */
  FORMAT_AGNOSTIC_FOR_ALL_INPUTS_AND_OUTPUTS = 1,

  /* FORMAT_AGNOSTIC_FOR_PAIRED_INPUT_AND_OUTPUT means this op can use arbitrary
   * format. And we need to set the format for each paired input and output as
   * the same. For example, the input0 and output0 should be the same format and
   * the input1 and output1 should be the same. Paired is definedas:
   * input:x and output:x and the size of inputs should be the same as the
   * size of outputs. */
  FORMAT_AGNOSTIC_FOR_PAIRED_INPUT_AND_OUTPUT = 2,

  /* FORMAT_PENETRATION means this op may not support heavy format but if we
   * distribute the heavy format through this op and the next op of this op
   * will be set as heavy format and the this op and the trans-nodes between
   * this op and the next op will be merged of fused. It will decrease the
   * number of transdata.
   * For example:
   * original graph: A(Fz) -> TransData(Fz->HWCN) -> B(HWCN),
   *
   * after penetrating TransData:
   * A(Fz) -> TransData(Fz->HWCN) -> B(Fz)
   *
   * after trans-nodes insertion:
   * A(Fz) -> TransData(Fz->HWCN) -> TransData(HWCN->Fz) -> B(Fz)
   *
   * after merging trans-nodes:
   * A(Fz) -> B(Fz), so the op TransData is defined as the FORMAT_PENETRATION.
   *
   * Another case:
   * original graph:
   *                const(NCHW)                                const(NCHW)
   *                /      \       ->(penetrate const)          /      \
   *            A(Fz)     B(NCHW)                           A(Fz)     B(Fz)
   * after trans-nodes insertion:
   *               const(NCHW)
   *              /            \
   *  TransData(NCHW->Fz)      TransData(NCHW->Fz)
   *        |                       |
   *      A(Fz)                   B(Fz)
   *
   * after trans-nodes and const fusion:
   *            const(Fz)
   *             /      \
   *          A(Fz)     B(Fz)
   */
  FORMAT_PENETRATION = 3,

  /* FORMAT_AGNOSTIC_FUNCTION_OP means this op is function op. */
  FORMAT_AGNOSTIC_FUNCTION_OP = 4,

  FORMAT_AGNOSTIC_BOTTOM = 5
};

struct PropagationInfo {
  int64_t group;            /* The value of attribute _fe_group of last node */
  std::string reshape_type; /* reshape type for reduce op */
  ge::Format heavy_format;
  int32_t sub_format;
  explicit PropagationInfo(int64_t group_value = 0)
      : group(group_value), reshape_type(""), heavy_format(ge::FORMAT_RESERVED), sub_format(0) {}
};

struct NodeInfo;
using NodeInfoPtr = std::shared_ptr<NodeInfo>;
struct NodeInfo {
  PropagationInfo propagation_info;
  ge::NodePtr current_node;
  OpKernelInfoPtr current_node_op_kernel_ptr;
  FormatSelectionType format_selection;
  int32_t anchor_index_of_curr_node;
  NodeInfoPtr last_node_info; /* record the last_node_info Ptr */
  ge::NodePtr last_node;
  bool is_sub_graph_data_or_nt_opt;
  bool is_input_of_curr_node;
  vector<IndexNameMap> tensor_map; /* the first map is for input and the second is for output */
  NodeInfo(ge::NodePtr node, NodeInfoPtr last_node_info_param, ge::Format heavy_format_param, int32_t sub_format_param,
           PropagationInfo &propagation_info_param, int32_t anchor_index_of_curr_node_param = 0,
           bool is_input_of_curr_node_param = true, bool is_sub_graph_data_or_nt_opt_param = false)
      : current_node(std::move(node)),
        anchor_index_of_curr_node(anchor_index_of_curr_node_param),
        last_node_info(last_node_info_param),
        is_sub_graph_data_or_nt_opt(is_sub_graph_data_or_nt_opt_param),
        is_input_of_curr_node(is_input_of_curr_node_param) {
    propagation_info.group = propagation_info_param.group;
    propagation_info.reshape_type = propagation_info_param.reshape_type;
    propagation_info.heavy_format = heavy_format_param;
    propagation_info.sub_format = sub_format_param;

    if (last_node_info_param == nullptr) {
      last_node = nullptr;
    } else {
      last_node = last_node_info_param->current_node;
    }
    current_node_op_kernel_ptr = nullptr;
    format_selection = FormatSelectionType::FORMAT_AGNOSTIC_BOTTOM;
  }
};

struct FormatAndDataTypeInfo {
  ge::Format heavy_format = ge::FORMAT_RESERVED;
  int32_t sub_format = 0;
  ge::DataType data_type = ge::DT_FLOAT;
  ge::OpDescPtr op_desc_ptr;
  ge::NodePtr node_ptr;
  int32_t anchor_index; /* Index of input or output anchor */
  int32_t format_index; /* The column of the selected format */
  IndexNameMap &tensor_map;
  InputOrOutputInfoPtr op_kernel_tensor_info;
  bool is_input;
  bool is_forward;
  bool is_sub_graph_data_or_nt_opt;
  FormatSelectionType format_selection_type;
  bool only_to_paired_input_or_output;
  PropagationInfo propagation_info;
  FormatAndDataTypeInfo(const ge::NodePtr &node_ptr_param, ge::OpDescPtr &op_desc_ptr_param, int32_t anchor_index_param,
                        int32_t format_index_param, bool is_input_param, bool is_forward_param,
                        FormatSelectionType format_selection_type_param, PropagationInfo &propagation_info_param,
                        IndexNameMap &tensor_map_param)
      : op_desc_ptr(op_desc_ptr_param),
        node_ptr(node_ptr_param),
        anchor_index(anchor_index_param),
        format_index(format_index_param),
        tensor_map(tensor_map_param),
        is_input(is_input_param),
        is_forward(is_forward_param) {
    propagation_info.reshape_type = propagation_info_param.reshape_type;
    propagation_info.group = propagation_info_param.group;
    propagation_info.heavy_format = propagation_info_param.heavy_format;
    propagation_info.sub_format = propagation_info_param.sub_format;

    format_selection_type = format_selection_type_param;
    only_to_paired_input_or_output = (format_selection_type ==
                                      FormatSelectionType::FORMAT_AGNOSTIC_FOR_PAIRED_INPUT_AND_OUTPUT);

    is_sub_graph_data_or_nt_opt = false;
  }
};

using TensorInfoPtr = std::shared_ptr<FormatAndDataTypeInfo>;
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;
using FormatDtypeSetterPtr = std::shared_ptr<FormatDtypeSetter>;
using HeavyFormatSelectorPtr = std::shared_ptr<HeavyFormatSelector>;
using HeavyFormatSupportFormatsUpdaterPtr = std::shared_ptr<HeavyFormatSupportFormatsUpdater>;
using RefRelationsPtr = std::shared_ptr<ge::RefRelations>;

class HeavyFormatPropagation {
 public:
  explicit HeavyFormatPropagation(const std::string &engine_name, OpStoreAdapterManagerPtr op_store_adapter_manager_ptr,
                                  RefRelationsPtr reflection_builder_ptr);

  ~HeavyFormatPropagation();

  HeavyFormatPropagation(const HeavyFormatPropagation &) = delete;

  HeavyFormatPropagation &operator=(const HeavyFormatPropagation &) = delete;

  Status Initialize();
  /**
   * The main function of distributing heavy format and it is also the
   * interface given to graph optimizer.
   * @return SUCCESS or FAIL
   */
  Status PropagateHeavyFormat(const ge::ComputeGraph &graph);

 private:
  /**
   * Propagate back ward from all input of current node to the peer outputs'
   * owner node.
   * @param last_node: we will not traversing to the node where the heavy
   * format is from.
   * @param format_index: The heavy format index in ops kernel
   * @param heavy_format: The heavy format which is propagated on the main
   * route
   * @return SUCCESS or FAIL
   */
  Status PropagateBackwards(const NodeInfoPtr &node_info, int32_t format_index,
                            FormatSelectionType format_selection_type, std::deque<NodeInfoPtr> &next_node_queue);
  /**
   * Propagate farward from all output of current node to the peer inputs'
   * owner node.
   * @param last_node: we will not traversing to the node where the heavy
   * format is from.
   * @param format_index: The heavy format index in ops kernel
   * @param heavy_format: The heavy format which is propagated on the main
   * route
   * @return SUCCESS or FAIL
   */
  Status PropagateForwards(const NodeInfoPtr &node_info, int32_t format_index,
                           FormatSelectionType format_selection_type, std::deque<NodeInfoPtr> &next_node_queue);

  Status RunPropagation(const NodeInfoPtr &node_info, std::deque<NodeInfoPtr> &next_node_queue);

  /* The heavy format of node_info may not passed to predecessors, because
   * each input/output's format may be different. so we use
   * format_of_current_tensor to represent the real format which is being
   * propagated. */
  Status PropagateFarther(const NodeInfoPtr &curr_node_info, const NodeInfoPtr &next_node_info,
                          std::deque<NodeInfoPtr> &next_node_queue);

  /**
   * Get Op kernel info from fe ops store
   * @return SUCCESS or FAIL
   */
  Status GetOpKernelInfo(const ge::NodePtr &current_node, OpKernelInfoPtr &op_kernel_info_ptr);

  Status SetInferFormat(const TensorInfoPtr &tensor_info_ptr);

  Status GetHeavyFormatIndex(const NodeInfoPtr &node_info, const HeavyFormatInfo &heavy_format_info,
                             int32_t &heavy_format_index) const;

  Status GetTensorKernelInfo(const ge::NodePtr &current_node, const TensorInfoPtr &tensor_info_ptr,
                             const OpKernelInfoPtr &op_kernel_info_ptr,
                             InputOrOutputInfoPtr &input_or_output_info) const;

  Status GetFormatAndDtypeFromOpKernel(const ge::NodePtr &current_node, const TensorInfoPtr &tensor_info_ptr,
                                       const OpKernelInfoPtr &op_kernel_info_ptr) const;

  Status SetFormats(const TensorInfoPtr &tensor_info_ptr, const OpKernelInfoPtr &op_kernel_info_ptr);

  Status SetNewShape(const TensorInfoPtr &tensor_info_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                     const ge::GeTensorDescPtr &current_tensor, ge::GeShape &original_shape, ge::GeShape &new_shape);

  Status SetFormatAndDTypeToOpdesc(const TensorInfoPtr &tensor_info_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                   Status set_format_result);
  bool IsHeavyFormat(const ge::Format &format) const;

  bool IsHeavyOp(const OpKernelInfoPtr &op_kernel_info_ptr);

 private:
  std::string engine_name_;

  std::vector<std::string> imply_type_string_vec_;

  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;

  RefRelationsPtr reflection_builder_ptr_;

  FormatDtypeQuerierPtr format_dtype_querier_ptr_;

  FormatDtypeSetterPtr format_dtype_setter_ptr_;

  HeavyFormatSelectorPtr heavy_format_selector_ptr_;

  HeavyFormatSupportFormatsUpdaterPtr supportformats_updater_ptr_;

  vector<NodeInfoPtr> format_agnostic_nodes_info_;
  /* If the op format agnostic type is
   * FORMAT_AGNOSTIC_FOR_ALL_INPUTS_AND_OUTPUTS, we still need to check
   * whether there is some input or output which is not format agnostic. */
  bool IsAnchorFormatAgnostic(const TensorInfoPtr &tensor_info_ptr, bool is_input, int64_t anchor_idex) const;
  /**
   * Propagate farward from all output of current node to the peer inputs'
   * owner node.
   * @param last_node: we will not traversing to the node where the heavy
   * format is from.
   * @param format_index: The heavy format index in ops kernel
   * @param heavy_format: The heavy format which is propagated on the main
   * route
   * @return SUCCESS or FAIL
   */
  Status PropagateNormalNodeForwards(const NodeInfoPtr &curr_node_info, int32_t format_index,
                                     const TensorInfoPtr &tensor_info_ptr, std::deque<NodeInfoPtr> &next_node_queue);

  /**
   * Propagate farward from all input of sub graph netoutput to peer inputs'
   * owner node.
   * @param last_node: we will not traversing to the node where the heavy
   * format is from.
   * @param format_index: The heavy format index in ops kernel
   * @param heavy_format: The heavy format which is propagated on the main
   * route
   * @return SUCCESS or FAIL
   */
  Status PropagateSubNetoutputForwards(const NodeInfoPtr &curr_node_info, int32_t format_index,
                                       const TensorInfoPtr &tensor_info_ptr, std::deque<NodeInfoPtr> &next_node_queue);

  /**
   * Propagate back ward from all input of current node to the peer outputs'
   * owner node.
   * @param last_node: we will not traversing to the node where the heavy
   * format is from.
   * @param format_index: The heavy format index in ops kernel
   * @param heavy_format: The heavy format which is propagated on the main
   * route
   * @return SUCCESS or FAIL
   */
  Status PropagateNormalNodeBackwards(const NodeInfoPtr &curr_node_info, int32_t format_index,
                                      TensorInfoPtr &tensor_info_ptr, std::deque<NodeInfoPtr> &next_node_queue);

  /**
   * Propagate back ward from all output of sub graph DATA to the peer outputs'
   * owner node.
   * @param last_node: we will not traversing to the node where the heavy
   * format is from.
   * @param format_index: The heavy format index in ops kernel
   * @param heavy_format: The heavy format which is propagated on the main
   * route
   * @return SUCCESS or FAIL
   */
  Status PropagateSubDataBackwards(const NodeInfoPtr &curr_node_info, const TensorInfoPtr &tensor_info_ptr,
                                   std::deque<NodeInfoPtr> &next_node_queue);

  Status GetNextNodesInfoForwards(std::deque<NodeInfoPtr> &next_node_queue,
                                  const ge::InDataAnchorPtr &peer_in_data_anchor, const ge::Format &heavy_format,
                                  const int32_t &sub_format, const NodeInfoPtr &last_node_info,
                                  PropagationInfo &propagation_info);

  Status GetNextNodesInfoBackWards(std::deque<NodeInfoPtr> &next_node_queue,
                                   const ge::OutDataAnchorPtr &peer_out_anchor, const ge::Format &heavy_format,
                                   const int32_t &sub_format, const NodeInfoPtr &last_node_info,
                                   PropagationInfo &propagation_info);

  std::string GetPropagationReshapeType(const TensorInfoPtr &tensor_info_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                        const ge::GeTensorDescPtr &current_tensor) const;

  Status SetReshapeType(const ge::OpDescPtr &op_desc_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                        const ge::Format &ori_format, const TensorInfoPtr &tensor_info_ptr) const;

  bool IsInputAvailable(const ge::OpDescPtr &op_desc_ptr, const ge::InDataAnchorPtr &in_data_anchor_ptr) const;

  bool IsOutputAvailable(const ge::OpDescPtr &op_desc_ptr, const ge::OutDataAnchorPtr &out_data_anchor_ptr) const;

  Status CheckSpecificSubGraphDataOrNetoutput(const TensorInfoPtr &tensor_info_ptr,
                                              bool &is_sub_graph_data_or_net_output,
                                              std::unordered_set<ge::RefCell, ge::RefCellHash> &reflections);

  Status FindSameNameVariableNodes(const ge::NodePtr &node_ptr, std::vector<ge::NodePtr> &var_nodes);

  bool CheckBackwardPropagtionNecessary(const NodeInfoPtr &curr_node_info, const ge::InDataAnchorPtr &in_data_anchor,
                                        const TensorInfoPtr &tensor_info_ptr, uint32_t &count_anchor);

  bool CheckForwardPropagtionNecessary(const NodeInfoPtr &curr_node_info, const ge::OpDescPtr &op_desc_ptr,
                                       const ge::OutDataAnchorPtr &out_data_anchor,
                                       const TensorInfoPtr &tensor_info_ptr);

  Status SetHeavyFormat(const ge::InDataAnchorPtr &in_data_anchor, const NodeInfoPtr &curr_node_info,
                        const ge::OpDescPtr &op_desc_ptr, const int32_t &format_index,
                        const TensorInfoPtr &tensor_info_ptr) const;

  bool IsHeavyFormatSupported(const ge::NodePtr &current_node, const NodeInfoPtr &next_node_info) const;

  /* Check whether the propagation will bring benefits to the whole network.
   * For variable, if all the successors can support the heavy format, we consider it as a worthy case.
   * For other nodes, the number of successors which do not support this heavy format is less than or equal to
   * 1, we will consider it as a worthy case. */
  bool JudgePropagationWorthy(const ge::NodePtr &node, const std::deque<NodeInfoPtr> &pending_next_nodes,
                              std::deque<NodeInfoPtr> &next_nodes);

  void AddNodeInfoToQueue(NodeInfoPtr &node_info, std::deque<NodeInfoPtr> &next_node_queue);

  void CreateNextNodeInfo(const ge::NodePtr &next_node, const NodeInfoPtr &last_node_info, ge::Format heavy_format,
                          int32_t sub_format, PropagationInfo &propagation_info, int32_t anchor_index, bool is_input,
                          NodeInfoPtr &next_node_info, std::deque<NodeInfoPtr> &next_node_queue);

  Status SetOpKernelAndTensorMap(const NodeInfoPtr &node_info);

  bool IsFormatAgnosticAnchor(const ge::NodePtr &current_node, const NodeInfoPtr &next_node) const;

  void SetFormatAgnosticType(const ge::OpDescPtr &op_desc_ptr, const NodeInfoPtr &node_info);

  Status UpdateForOneNode(const NodeInfoPtr &node_info) const;

  Status UpdateInputAndOutputForFormatContinuous();

  bool IsNextNodeQualified(const ge::NodePtr &current_node, const NodeInfoPtr &next_node);

  bool CheckSwitchQualified(const std::vector<NodeInfoPtr> &node_list);

  bool IsConstNodeInSubGraph(const ge::NodePtr &node_ptr) const;

  void RollBackFormatAndShape(const ge::NodePtr &node, bool is_sub_graph_weight) const;

  Status UpdateInputForAllType(ge::NodePtr &current_node, const vector<int64_t> &input_non_format_agnostic_index,
                               const vector<int64_t> &output_non_format_agnostic_index) const;

  Status UpdateOutputForAllType(const ge::NodePtr &current_node, const vector<int64_t> &input_non_format_agnostic_index,
                                const vector<int64_t> &output_non_format_agnostic_index) const;

  Status UpdateInputForPairType(const ge::NodePtr &current_node, const vector<int64_t> &input_non_format_agnostic_index,
                                const vector<int64_t> &output_non_format_agnostic_index) const;

  Status UpdateOutputForPairType(const ge::NodePtr &current_node,
                                 const vector<int64_t> &input_non_format_agnostic_index,
                                 const vector<int64_t> &output_non_format_agnostic_index) const;

  void UpdateInputFormatAndShape(ge::NodePtr &current_node, const vector<int64_t> &input_non_format_agnostic_index,
                                 const vector<int64_t> &output_non_format_agnostic_index, const ge::Format &format,
                                 const int32_t &sub_format, const ge::GeShape &shape) const;

  void UpdateOutputFormatAndShape(const ge::NodePtr &current_node,
                                  const vector<int64_t> &input_non_format_agnostic_index,
                                  const vector<int64_t> &output_non_format_agnostic_index, const ge::Format &format,
                                  const int32_t &sub_format, const ge::GeShape &shape) const;

  bool CheckFormatCompatible(const ge::Format &primary_format, const ge::Format &origin_format) const;
};

}  // namespace fe
#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_HEAVY_FORMAT_PROPAGATION_HEAVY_FORMAT_PROPAGATION_H_