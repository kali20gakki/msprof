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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_OP_FORMAT_DTYPE_JUDGE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_OP_FORMAT_DTYPE_JUDGE_H_

#include "format_selector/manager/format_dtype_querier.h"
#include "graph_optimizer/op_judge/format_and_dtype/strategy/op_format_dtype_strategy_manager.h"
#include "graph_optimizer/op_judge/format_and_dtype/update_desc/op_format_dtype_update_desc.h"
#include "graph_optimizer/op_judge/format_and_dtype/update_desc/subgraph/sub_data_format_dtype_update.h"
#include "graph_optimizer/op_judge/format_and_dtype/update_desc/subgraph/sub_netoutput_format_dtype_update.h"
#include "graph_optimizer/op_judge/op_judge_base.h"

namespace fe {

using OpFormatDtypeStrategyManagerPtr = std::shared_ptr<OpFormatDtypeStrategyManager>;
using OpFormatDtypeUpdateDescPtr = std::shared_ptr<OpFormatDtypeUpdateDesc>;
using FormatDtypeQuerierPtr = std::shared_ptr<FormatDtypeQuerier>;
using SubDataFormatDtypeUpdatePtr = std::shared_ptr<SubDataFormatDtypeUpdate>;
using SubNetOutputFormatDtypeUpdatePtr = std::shared_ptr<SubNetOutputFormatDtypeUpdate>;

class OpFormatDtypeJudge : public OpJudgeBase {
 public:
  OpFormatDtypeJudge(const std::string& engine_name,
                     OpStoreAdapterManagerPtr op_store_adapter_manager_ptr, RefRelationsPtr reflection_builder_ptr);

  virtual ~OpFormatDtypeJudge() override;

  Status Judge(ge::ComputeGraph &graph) override;

  Status Initialize();
 private:
  /**
   * judge the format and data type for the node
   * @param node_ptr current node
   * @return SUCCESS or FAIL
   */
  Status JudgeByNode(ge::NodePtr node_ptr);
  /**
   * set the highest prior imply type of op, update the format and data type of
   * the input or output desc of the current_node
   * @param node_ptr current node
   * @param imply_type_str imply type
   * @return SUCCESS or FAIL
   */
  Status SetDtypeAndFormatByPrecisionMode(ge::NodePtr node_ptr, const std::string &imply_type_str);

  /**
   * Get all possible input data type index by specific precision mode
   * @param node_ptr: current node
   * @param op_kernel_info_ptr: op kernel information
   * @param input_map: the index name map of tensors
   * @param prio_index_map: this is the sorted tensor index, we will loop among
   * tensors according to the sequence of this map
   * @param matched_index_vec: the vector which stores matched dtype index
   * @return SUCCESS or FAIL
   */
  Status GetInputAndOutputDtypeIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                     const IndexNameMap &input_map, const IndexNameMap &output_map,
                                     const std::map<uint32_t, int> &prio_index_map,
                                     vector<uint32_t> &matched_index_vec);

  /**
   * Get all possible input data type index by specific precision mode
   * @param node_ptr: current node
   * @param op_kernel_info_ptr: op kernel information
   * @param output_map: the index name map of tensors
   * @param prio_index_map: this is the sorted tensor index, we will loop among
   * tensors according to the sequence of this map
   * @param matched_index_vec: the vector which stores matched dtype index
   * @return SUCCESS or FAIL
   */
  Status GetInputAndOutputFormatIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                      const IndexNameMap &input_map, const IndexNameMap &output_map,
                                      const std::map<uint32_t, int> &prio_index_map,
                                      vector<uint32_t> &matched_index_vec);

  /**
   * sort the input desc: non const/constant/variable inputs first
   * @param node_ptr current node
   * @param op_desc_ptr current op desc
   * @param key is priority, value is the input index
   * @return SUCCESS or FAIL
   */
  Status SortInputBySequence(ge::NodePtr node_ptr, ge::OpDescPtr op_desc_ptr, std::map<uint32_t, int> &prio_index_map);

  /**
   * Get all possible input data type index by specific precision mode
   * @param node_ptr: current node
   * @param op_kernel_info_ptr: op kernel information
   * @param input_map: the index name map of tensors
   * @param prio_index_map: this is the sorted tensor index, we will loop among
   * tensors according to the sequence of this map
   * @param matched_index_vec: the vector which stores matched dtype index
   * @return SUCCESS or FAIL
   */
  Status GetInputDtypeIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                            const IndexNameMap &input_map, const std::map<uint32_t, int> &prio_index_map,
                            vector<uint32_t> &matched_index_vec);

  /**
   * Get all possible input data type index by specific precision mode
   * @param node_ptr: current node
   * @param op_kernel_info_ptr: op kernel information
   * @param input_map: the index name map of tensors
   * @param prio_index_map: this is the sorted tensor index, we will loop among
   * tensors according to the sequence of this map
   * @param matched_index_vec: the vector which stores matched dtype index
   * @return SUCCESS or FAIL
   */
  Status GetInputFormatIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                             const IndexNameMap &input_map, const std::map<uint32_t, int> &prio_index_map,
                             vector<uint32_t> &matched_index_vec);

  /**
   * Get all possible input data type index by specific precision mode
   * @param node_ptr: current node
   * @param op_kernel_info_ptr: op kernel information
   * @param input_map: the index name map of tensors
   * @param matched_index_vec: the vector which stores matched dtype index
   * @return SUCCESS or FAIL
   */
  Status GetOutputDtypeIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                             const IndexNameMap &output_map, vector<uint32_t> &matched_index_vec);

  /**
   * Get all possible input data type index by specific precision mode
   * @param node_ptr: current node
   * @param op_kernel_info_ptr: op kernel information
   * @param output_map: the index name map of tensors
   * @param matched_index_vec: the vector which stores matched dtype index
   * @return SUCCESS or FAIL
   */
  Status GetOutputFormatIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                              const IndexNameMap &output_map, vector<uint32_t> &matched_index_vec);

  /**
   * whether to judge the format. The Data, Const, Variable,
   * NetOutput and non fe imply type don't need to judge the format.
   * @param op_desc_ptr currrent op desc
   * @param imply_type the imply type of the op_desc_ptr
   * @return result
   */
  bool IsNoNeedJudge(const ge::OpDescPtr &op_desc_ptr, int &imply_type) const;

  uint32_t GetMatchedIndex(const vector<uint32_t> &matched_index_vec,
                           const string &op_name, const string &op_type) const;

  OpStoreAdapterManagerPtr op_store_adapter_manager_ptr_;
  RefRelationsPtr reflection_builder_ptr_;
  OpFormatDtypeStrategyManagerPtr op_format_dtype_strategy_manager_ptr_;
  OpFormatDtypeUpdateDescPtr op_format_dtype_update_desc_ptr_;
  FormatDtypeQuerierPtr format_dtype_querier_ptr_;
  SubDataFormatDtypeUpdatePtr sub_data_format_dtype_update_ptr_;
  SubNetOutputFormatDtypeUpdatePtr sub_net_output_format_dtype_update_ptr_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_JUDGE_FORMAT_AND_DTYPE_OP_FORMAT_DTYPE_JUDGE_H_