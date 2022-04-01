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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_PSROIPOOLING_FUSION_PASS_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_PSROIPOOLING_FUSION_PASS_H_

#include "graph_optimizer/fusion_common/pattern_fusion_base_pass.h"

namespace fe {
class PSROIPoolingFusionPass : public PatternFusionBasePass {
 public:
  PSROIPoolingFusionPass();
  ~PSROIPoolingFusionPass() override;

 protected:
  vector<FusionPattern *> DefinePatterns() override;
  Status Fusion(ge::ComputeGraph &graph, Mapping &mapping, vector<ge::NodePtr> &new_nodes) override;

 private:
  /**
   * Do SwapCi fusion for PSROIPooling
   * @param graph: original graph info
   * @param pre_node_ptr: pre node of PSROIPooling
   * @param psroi_node_ptr: PSROIPooling node
   * @param new_nodes: new nodes after fusion
   * @return SUCCESS/FAILED
   */
  Status SwapCiFuison(ge::ComputeGraph &graph, const ge::NodePtr &pre_node_ptr,
                      const ge::NodePtr &psroi_node_ptr,
                      vector<ge::NodePtr> &new_nodes);

  /**
   * Do SwapCo fusion for PSROIPooling
   * @param graph: original graph info
   * @param conv_node_ptr: conv2d node info
   * @param psroi_node_ptr: PSROIPooling node
   * @param new_nodes: new nodes after fusion
   * @return SUCCESS/FAILED
   */
  Status SwapCoFuison(ge::ComputeGraph &graph, const ge::NodePtr &conv_node_ptr,
                      const ge::NodePtr &psroi_node_ptr,
                      vector<ge::NodePtr> &new_nodes);

  /**
   * Check parameters of PSROIPooling right or not
   * @param psroi_node_ptr: PSROIPooling node
   * @return SUCCESS/FAILED
   */
  Status CheckParameter(const ge::NodePtr &psroi_node_ptr);

  /**
   * Set output_dim and group_size attr value
   * @param new_node_ptr: new node
   * @return SUCCESS/FAILED
   */
  Status SetAttrValueForNewNode(const ge::OpDescPtr &psroi_op_desc_ptr, const ge::OpDescPtr &new_op_desc_ptr) const;

  /**
   * Get new input desc info of SwapCo or SwapCi
   * @param current_op_desc_ptr: current op desc(SwapCo/SwapCi)
   * @param pre_op_desc_ptr: pre op desc
   * @param input_tensor_desc: old input desc of SwapCo or SwapCi
   * @return SUCCESS/FAILED
   */
  Status GetSwapInputTensorDesc(const ge::OpDescPtr &pre_op_desc_ptr,
                                ge::GeTensorDesc &input_tensor_desc) const;

  /**
   * Get new input desc info of SwapCi
   * @param current_op_desc_ptr: current op desc(SwapCi)
   * @param next_op_desc_ptr: next op of PSROIPooling
   * @param input_tensor_desc: input desc of SwapCi
   * @param output_tensor_desc: new out desc of SwapCi
   * @return SUCCESS/FAILED
   */
  Status GetSwapCiOutputTensorDesc(const ge::OpDescPtr &next_op_desc_ptr,
                                   const ge::GeTensorDesc &input_tensor_desc, ge::GeTensorDesc &output_tensor_desc);

  /**
   * Get new output desc info of SwapCo
   * @param current_op_desc_ptr: current op desc(SwapCo)
   * @param next_op_desc_ptr: next op of PSROIPooling
   * @param input_tensor_desc: input desc of SwapCo
   * @param output_tensor_desc: new out desc of SwapCo
   * @return SUCCESS/FAILED
   */
  Status GetSwapCoOutputTensorDesc(const ge::OpDescPtr &next_op_desc_ptr,
                                   const ge::GeTensorDesc &input_tensor_desc, const uint32_t &weight_index,
                                   ge::GeTensorDesc &output_tensor_desc);

  /**
   * Update input desc of PSROIPooling
   * @param psroip_desc_ptr: desc info of PSROIPooling
   * @param swap_output_tensor_desc: output desc of SwapCo or SwapCi
   * @return SUCCESS/FAILED
   */
  Status UpdatePSROIInput(const ge::OpDescPtr &psroi_op_desc_ptr,
                          const ge::GeTensorDesc &swap_output_tensor_desc) const;

  /**
   * Update output desc of PSROIPooling, only when SwapCi fusion
   * @param psroi_op_desc_ptr: desc info of PSROIPooling
   * @param swap_ci_output_tensor_desc
   * @return SUCCESS/FAILED
   */
  Status UpdatePSROIOutput(ge::OpDescPtr &psroi_op_desc_ptr);

  /**
   * Update weight desc and output desc of Conv2D
   * @param conv_op_desc_ptr: conv desc info
   * @param swap_co_output_tensor_desc: output desc info of SwapCo
   * @return SUCCESS/FAILED
   */
  Status UpdateConv2DWeightAndOut(const ge::OpDescPtr &conv_op_desc_ptr, const uint32_t &weight_index,
                                  const ge::GeTensorDesc &swap_co_output_tensor_desc) const;

  /**
   * Get new shape according to old format and old shape and new format
   * @param old_format: old format
   * @param old_shape: old shape
   * @param new_format: new format
   * @param data_type: data type of current op input or output
   * @return new shape
   */
  ge::GeShape GetInOrOutputNewShape(const ge::Format &old_format, const ge::GeShape &old_shape,
                                    const ge::Format &new_format, const ge::DataType &data_type) const;

  /**
   * Get new shape after supplement to output_dim
   * @param old_format: old format
   * @param old_shape: new shape
   * @param output_dim: value of output_dim
   * @param fold_axis: axis name
   * @return new shape
   */
  ge::GeShape GetNewShapeFolding(const ge::Format &old_format, const ge::GeShape &old_shape, const int64_t &output_dim,
                                 const int32_t &fold_axis) const;

  /**
   * Get a min multiple value of a base num
   * @param base_num: base num
   * @param multiple: multiple
   * @return min value
   */
  int64_t GetMixAliquotsNum(const int64_t &base_num, const int32_t &multiple) const;

  int64_t output_dim_;
  int64_t group_size_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_GRAPH_FUSION_FUSION_PASS_MANAGER_BUILTIN_PASS_PSROIPOOLING_FUSION_PASS_H_
