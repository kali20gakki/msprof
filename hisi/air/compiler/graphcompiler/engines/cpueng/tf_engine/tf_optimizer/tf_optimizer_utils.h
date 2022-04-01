/**
 * Copyright 2020 Huawei Technologies Co., Ltd
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
#ifndef AICPU_TF_OPTIMIZER_UTILS_H_
#define AICPU_TF_OPTIMIZER_UTILS_H_

#include "graph/compute_graph.h"
#include "external/ge/ge_api_error_codes.h"
#include "common/sgt_slice_type.h"
namespace aicpu {
class TfVariableGraph {
 public:
  /**
   * Generate the graph after inserte CacheUpdate node
   * @param graph Compute graph
   * @return status whether this operation success
   */
  static ge::Status GenerateTfVariableGraph(ge::ComputeGraph &graph);

 private:
  /**
   * Create assign node
   * @param first_src_tensor_desc first src Tensor Desc
   * @param second_src_tensor_desc second src Tensor Desc
   * @param dst_tensor_desc dst Tensor Desc
   * @param graph Compute graph
   * @param assign_node Assign node ptr
   * @return status whether this operation success
   */
  static ge::Status CreateAssign(ge::GeTensorDesc &first_src_tensor_desc,
                                 ge::GeTensorDesc &second_src_tensor_desc,
                                 ge::GeTensorDesc &dst_tensor_desc,
                                 ge::ComputeGraph &graph,
                                 ge::NodePtr &assign_node);

  /**
   * Create identity node
   * @param src_tensor_desc src Tensor Desc
   * @param dst_tensor_desc dst Tensor Desc
   * @param graph Compute graph
   * @param identity_node Indentity node ptr
   * @return status whether this operation success
   */
  static ge::Status CreateIdentity(ge::GeTensorDesc &src_tensor_desc,
                                   ge::GeTensorDesc &dst_tensor_desc,
                                   ge::ComputeGraph &graph,
                                   ge::NodePtr &identity_node);

  /**
   * Create Variable node
   * @param dst_tensor_desc dst Tensor Desc
   * @param graph Compute graph
   * @param variable_node Variable node ptr
   * @return status whether this operation success
   */
  static ge::Status CreateVariable(ge::GeTensorDesc &dst_tensor_desc,
                                   ge::ComputeGraph &graph,
                                   ge::NodePtr &variable_node);

  /**
   * Insert Variable node
   * @param src_anchor Src anchor ptr
   * @param dst_anchor Dst anchor ptr
   * @param variable_node Variable node ptr
   * @return status whether this operation success
   */
  static ge::Status InsertVariable(ge::OutDataAnchorPtr &src_anchor,
                                   const ge::InDataAnchorPtr &dst_anchor,
                                   ge::NodePtr &variable_node);

  /**
   * Insert assign node
   * @param src_anchor Src anchor ptr
   * @param dst_anchor Dst anchor ptr
   * @param node_ptr Node ptr
   * @param assign_node Assign node ptr
   * @return status whether this operation success
   */
  static ge::Status InsertAssign(ge::OutDataAnchorPtr &src_anchor,
                                 const ge::InDataAnchorPtr &dst_anchor,
                                 const ge::NodePtr &node_ptr,
                                 ge::NodePtr &assign_node);

  /**
   * Insert identity node
   * @param src_anchor Src anchor ptr
   * @param dst_anchor Dst anchor ptr
   * @param identity_node Identity node ptr
   * @return status whether this operation success
   */
  static ge::Status InsertIdentity(ge::OutDataAnchorPtr &src_anchor,
                                   const ge::InDataAnchorPtr &dst_anchor,
                                   ge::NodePtr &identity_node);

  /**
   * Create and insert the variable node in graph
   * @param src_anchor Src anchor ptr, need update after insert
   * @param dst_anchor Dst anchor ptr
   * @param variable_node Variable node ptr
   * @param graph Compute graph
   * @return status whether this operation success
   */
  static ge::Status CreateAndInsertVariable(ge::OutDataAnchorPtr &src_anchor,
                                            const ge::InDataAnchorPtr &dst_anchor,
                                            ge::NodePtr &variable_node,
                                            ge::ComputeGraph &graph);

  /**
   * Create and insert the identity node in graph
   * @param src_anchor Src anchor ptr, need update after insert
   * @param dst_anchor Dst anchor ptr
   * @param graph Compute graph
   * @return status whether this operation success
   */
  static ge::Status CreateAndInsertIdentity(ge::OutDataAnchorPtr &src_anchor,
                                            const ge::InDataAnchorPtr &dst_anchor,
                                            ge::ComputeGraph &graph);

  /**
   * Create and insert the Assign node in graph
   * @param src_anchor Src anchor ptr
   * @param dst_anchor Dst anchor ptr
   * @param node_ptr Assign target Op Node(eg. Variable Op)
   * @param graph Compute graph
   * @return status whether this operation success
   */
  static ge::Status CreateAndInsertAssign(ge::OutDataAnchorPtr &src_anchor,
                                          const ge::InDataAnchorPtr &dst_anchor,
                                          const ge::NodePtr &node_ptr,
                                          ge::ComputeGraph &graph);

  /**
   * Check Tensor is ref or not
   * @param tensor_desc tensor desc
   * @return bool if is ref tensor
   */
  static bool IsRefTensorDesc(const ge::GeTensorDesc &tensor_desc);
};
} // namespace aicpu
#endif // AICPU_TF_OPTIMIZER_UTILS_H_