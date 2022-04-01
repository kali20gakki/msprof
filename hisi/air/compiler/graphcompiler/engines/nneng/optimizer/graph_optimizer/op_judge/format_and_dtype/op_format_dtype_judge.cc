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

#include "graph_optimizer/op_judge/format_and_dtype/op_format_dtype_judge.h"
#include <utility>
#include "common/fe_inner_attr_define.h"
#include "common/fe_utils.h"
#include "common/graph/fe_graph_utils.h"
#include "common/op_info_common.h"
#include "common/util/op_info_util.h"
#include "common/configuration.h"
#include "ops_store/ops_kernel_manager.h"
#include "graph/compute_graph.h"
#include "graph/debug/ge_attr_define.h"
#include "graph/ge_attr_value.h"
#include "graph/utils/attr_utils.h"
#include "graph/utils/op_desc_utils.h"
#include "graph/utils/tensor_utils.h"
#include "graph_optimizer/shape_format_transfer/transfer_shape_according_to_format.h"

namespace fe {
OpFormatDtypeJudge::OpFormatDtypeJudge(const std::string& engine_name,
                                       OpStoreAdapterManagerPtr op_store_adapter_manager_ptr,
                                       RefRelationsPtr reflection_builder_ptr)
    : OpJudgeBase(engine_name),
      op_store_adapter_manager_ptr_(op_store_adapter_manager_ptr),
      reflection_builder_ptr_(reflection_builder_ptr),
      op_format_dtype_strategy_manager_ptr_(nullptr),
      op_format_dtype_update_desc_ptr_(nullptr),
      format_dtype_querier_ptr_(nullptr),
      sub_data_format_dtype_update_ptr_(nullptr),
      sub_net_output_format_dtype_update_ptr_(nullptr) {}

OpFormatDtypeJudge::~OpFormatDtypeJudge() {}

Status OpFormatDtypeJudge::Initialize() {
  FE_MAKE_SHARED(format_dtype_querier_ptr_ = std::make_shared<FormatDtypeQuerier>(op_store_adapter_manager_ptr_),
                 return FAILED);
  FE_CHECK_NOTNULL(format_dtype_querier_ptr_);

  FE_MAKE_SHARED(op_format_dtype_strategy_manager_ptr_ = std::make_shared<OpFormatDtypeStrategyManager>(engine_name_,
    format_dtype_querier_ptr_), return FAILED);
  FE_CHECK_NOTNULL(op_format_dtype_strategy_manager_ptr_);
  Status ret = op_format_dtype_strategy_manager_ptr_->Initialize();
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][Init] Failed to initialized OpFormatDtypeStrategyManager!");
    return ret;
  }

  FE_MAKE_SHARED(op_format_dtype_update_desc_ptr_ =
                 std::make_shared<OpFormatDtypeUpdateDesc>(format_dtype_querier_ptr_), return FAILED);
  FE_CHECK_NOTNULL(op_format_dtype_update_desc_ptr_);
  ret = op_format_dtype_update_desc_ptr_->Initialize();
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][Init] Failed to initialized OpFormatDtypeUpdateDesc!");
    return ret;
  }

  FE_MAKE_SHARED(sub_data_format_dtype_update_ptr_ =
                 std::make_shared<SubDataFormatDtypeUpdate>(reflection_builder_ptr_), return FAILED);
  FE_CHECK_NOTNULL(sub_data_format_dtype_update_ptr_);

  FE_MAKE_SHARED(sub_net_output_format_dtype_update_ptr_ =
                 std::make_shared<SubNetOutputFormatDtypeUpdate>(reflection_builder_ptr_), return FAILED);
  FE_CHECK_NOTNULL(sub_net_output_format_dtype_update_ptr_);

  return SUCCESS;
}

Status OpFormatDtypeJudge::Judge(ge::ComputeGraph &graph) {
  FE_TIMECOST_START(OpFormatDtypeJudge);
  for (auto &node : graph.GetAllNodes()) {
    Status status = JudgeByNode(node);
    if (status != SUCCESS) {
      return status;
    }
  }
  FE_TIMECOST_END(OpFormatDtypeJudge, "OpFormatDtypeJudge during FEGraphOptimizer::OptimizeOriginalGraph");
  return SUCCESS;
}

Status OpFormatDtypeJudge::JudgeByNode(ge::NodePtr node_ptr) {
  FE_CHECK_NOTNULL(node_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  auto owner_graph = node_ptr->GetOwnerComputeGraph();
  FE_CHECK_NOTNULL(owner_graph);

  string graph_name = owner_graph->GetName();
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();

  // 1. subgraph data
  if (FeGraphUtils::IsSubGraphData(op_desc_ptr)) {
    FE_LOGD("Graph[%s]Op[name=%s,type=%s]: the node is the data of the subgraph.", graph_name.c_str(), op_name.c_str(),
            op_type.c_str());
    (void)sub_data_format_dtype_update_ptr_->UpdateTensorDesc(node_ptr);
  }

  // 2. subgraph netoutput
  if (FeGraphUtils::IsSubGraphNetOutput(op_desc_ptr)) {
    FE_LOGD("Graph[%s]Op[name=%s,type=%s]: the node is the netoutput of the subgraph.", graph_name.c_str(),
            op_name.c_str(), op_type.c_str());
    (void)sub_net_output_format_dtype_update_ptr_->UpdateTensorDesc(node_ptr);
  }

  // 3. no need judge
  int imply_type = -1;
  if (IsNoNeedJudge(op_desc_ptr, imply_type)) {
    return SUCCESS;
  }

  // 5. get op kernel info store name
  FE_LOGD("Graph[%s]Op[name=%s,type=%s]: judge the format and the dtype.", graph_name.c_str(), op_name.c_str(),
          op_type.c_str());
  OpImplType op_imply_type = static_cast<OpImplType>(imply_type);
  FEOpsStoreInfo op_store_info;
  if (Configuration::Instance(engine_name_).GetOpStoreInfoByImplType(op_imply_type, op_store_info) != SUCCESS) {
    FE_LOGW("Engine[%s]Op[name=%s,type=%s]: get the op store info by imply_type %d failed.",
            engine_name_.c_str(), op_name.c_str(), op_type.c_str(), op_imply_type);
    return SUCCESS;
  }
  string imply_type_str = op_store_info.fe_ops_store_name;

  // 6. set input and output data type and format
  Status ret = SetDtypeAndFormatByPrecisionMode(node_ptr, imply_type_str);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][JdgByNd] Graph[%s]Op[name=%s,type=%s]: \
                    SetDtypeAndFormatByPrecisionMode failed, the implyType is %s",
                    graph_name.c_str(), op_name.c_str(), op_type.c_str(), imply_type_str.c_str());
    return ret;
  }

  return SUCCESS;
}

uint32_t OpFormatDtypeJudge::GetMatchedIndex(const vector<uint32_t> &matched_index_vec, const string &op_name,
                                             const string &op_type) const {
  if (matched_index_vec.empty()) {
    // no matching dtype or format for this op, so we pick the first column of
    // dtype and format
    FE_LOGW("Op[name=%s,type=%s]: the dtype and format is different from the op store, set the matched index to be 0.",
        op_name.c_str(), op_type.c_str());
    return 0;
  } else {
    FE_LOGD(
        "Op[name=%s,type=%s]: the size of the matched_index_vec is %zu,"
        "get the first matched index %u.",
        op_name.c_str(), op_type.c_str(), matched_index_vec.size(), matched_index_vec[0]);
    return matched_index_vec[0];
  }
}

Status OpFormatDtypeJudge::SetDtypeAndFormatByPrecisionMode(ge::NodePtr node_ptr, const std::string &imply_type_str) {
  FE_CHECK_NOTNULL(node_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);

  // 1. get the op_kernel_info_ptr
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  OpKernelInfoPtr op_kernel_info_ptr =
          OpsKernelManager::Instance(engine_name_).GetOpKernelInfoByOpType(imply_type_str, op_desc_ptr->GetType());
  if (op_kernel_info_ptr == nullptr) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][SetDtypeFmtByPrecisMode] Op[name=%s,type=%s]: \
                    GetOpKernelInfoByOpType failed, the imply type is %s.", op_name.c_str(),
                    op_type.c_str(), imply_type_str.c_str());
    return FAILED;
  }

  FE_CHECK_NOTNULL(op_kernel_info_ptr);

  // 2. get input index name map
  IndexNameMap input_index_map;
  Status ret = GetInputIndexNameMap(*(op_desc_ptr.get()), *op_kernel_info_ptr, input_index_map);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][SetDtypeFmtByPrecisMode] Op[name=%s,type=%s]: \
                    GetInputIndexNameMap failed.", op_name.c_str(), op_type.c_str());
    return ret;
  }

  // 3. get output index name map
  IndexNameMap output_index_map;
  ret = GetOutputIndexNameMap(*(op_desc_ptr.get()), *op_kernel_info_ptr, output_index_map);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][SetDtypeFmtByPrecisMode] Op[name=%s,type=%s]: \
                    GetOutputIndexNameMap failed.", op_name.c_str(), op_type.c_str());
    return ret;
  }

  // 4. get the matched_index_vec according to inputs
  vector<uint32_t> matched_index_vec;
  // 5. sort inputs
  std::map<uint32_t, int> prio_index_map;
  SortInputBySequence(node_ptr, op_desc_ptr, prio_index_map);
  // 6. get all supported data type index of all inputs and outputs first
  ret = GetInputAndOutputDtypeIndex(node_ptr, op_kernel_info_ptr, input_index_map,
                                    output_index_map, prio_index_map, matched_index_vec);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOpt][FmtJdg][SetDtypeFmtByPrecisMode][Op %s,type=%s]: failed to get format and dtype index.",
                    op_name.c_str(), op_type.c_str());
    return ret;
  }

  /* 7. get supported format index by original format then */
  ret = GetInputAndOutputFormatIndex(node_ptr, op_kernel_info_ptr, input_index_map,
                                     output_index_map, prio_index_map, matched_index_vec);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][SetDtypeFmtByPrecisMode] Op[name=%s,type=%s]: \
                    GetOutputFormatAndDtypeIndex failed.", op_name.c_str(), op_type.c_str());
    return ret;
  }

  // 8. get the matched index
  uint32_t matched_index = GetMatchedIndex(matched_index_vec, op_name, op_type);

  // 9. update the input and output desc of the node
  ret = op_format_dtype_update_desc_ptr_->UpdateTensorDescInfo(op_kernel_info_ptr, matched_index,
                                                               input_index_map, true, node_ptr);
  if (ret != SUCCESS) {
    REPORT_FE_ERROR("[GraphOptJdgInst][UpdFmtAndDtype][SetDtypeFmtByPrecisMode] Failed to update input of %s",
                    node_ptr->GetName().c_str());
    return ret;
  }
  ret = op_format_dtype_update_desc_ptr_->UpdateTensorDescInfo(op_kernel_info_ptr, matched_index, output_index_map,
                                                               false, node_ptr);
  return ret;
}

Status OpFormatDtypeJudge::SortInputBySequence(ge::NodePtr node_ptr, ge::OpDescPtr op_desc_ptr,
                                               std::map<uint32_t, int> &prio_index_map) {
  uint32_t default_prio = 0;
  uint32_t prio_gap = 0xff;
  uint32_t final_prio;
  uint32_t input_size = op_desc_ptr->GetInputsSize();
  for (uint32_t i = 0; i < input_size; ++i) {
    auto in_anchor = node_ptr->GetInDataAnchor(i);
    if (in_anchor == nullptr || in_anchor->GetPeerOutAnchor() == nullptr ||
        in_anchor->GetPeerOutAnchor()->GetOwnerNode() == nullptr ||
        in_anchor->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc() == nullptr) {
      FE_UINT32_ADDCHECK(prio_gap, default_prio);
      final_prio = prio_gap + default_prio;
      prio_index_map.emplace(std::make_pair(final_prio, i));
      ++default_prio;
      FE_LOGW("Op[name=%s,type=%s]: the input anchor %u is invalid.", node_ptr->GetName().c_str(),
              node_ptr->GetType().c_str(), i);
    } else {
      auto peer_out_op_type = in_anchor->GetPeerOutAnchor()->GetOwnerNode()->GetOpDesc()->GetType();
      if (peer_out_op_type == CONSTANT || peer_out_op_type == CONSTANTOP || peer_out_op_type == VARIABLE) {
        FE_UINT32_ADDCHECK(prio_gap, default_prio);
        final_prio = default_prio + prio_gap;
      } else {
        final_prio = default_prio;
      }
      prio_index_map.emplace(std::make_pair(final_prio, i));
      ++default_prio;
    }
  }

  if (prio_index_map.empty()) {
    for (uint32_t i = 0; i < input_size; ++i) {
      FE_UINT32_ADDCHECK(default_prio, 1);
      prio_index_map.emplace(default_prio++, i);
    }
  }
  FE_LOGD("Op[name=%s,type=%s]: after sorting, the total input size is %zu.", node_ptr->GetName().c_str(),
          node_ptr->GetType().c_str(), prio_index_map.size());
  return SUCCESS;
}

Status OpFormatDtypeJudge::GetInputAndOutputDtypeIndex(const ge::NodePtr &node_ptr,
                                                       const OpKernelInfoPtr &op_kernel_info_ptr,
                                                       const IndexNameMap &input_map, const IndexNameMap &output_map,
                                                       const std::map<uint32_t, int> &prio_index_map,
                                                       vector<uint32_t> &matched_index_vec) {
  Status ret = GetInputDtypeIndex(node_ptr, op_kernel_info_ptr, input_map, prio_index_map, matched_index_vec);
  if (ret != SUCCESS) {
    return ret;
  } else {
    return GetOutputDtypeIndex(node_ptr, op_kernel_info_ptr, output_map, matched_index_vec);
  }
}

Status OpFormatDtypeJudge::GetInputAndOutputFormatIndex(const ge::NodePtr &node_ptr,
                                                        const OpKernelInfoPtr &op_kernel_info_ptr,
                                                        const IndexNameMap &input_map, const IndexNameMap &output_map,
                                                        const std::map<uint32_t, int> &prio_index_map,
                                                        vector<uint32_t> &matched_index_vec) {
  Status ret = GetInputFormatIndex(node_ptr, op_kernel_info_ptr, input_map, prio_index_map, matched_index_vec);
  if (ret != SUCCESS) {
    return ret;
  } else {
    return GetOutputFormatIndex(node_ptr, op_kernel_info_ptr, output_map, matched_index_vec);
  }
}

Status OpFormatDtypeJudge::GetInputDtypeIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                              const IndexNameMap &input_map,
                                              const std::map<uint32_t, int> &prio_index_map,
                                              vector<uint32_t> &matched_index_vec) {
  FE_CHECK_NOTNULL(node_ptr);
  FE_CHECK_NOTNULL(op_kernel_info_ptr);
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  FE_CHECK_NOTNULL(op_desc_ptr);
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  uint32_t input_size = op_desc_ptr->GetInputsSize();
  FE_LOGD("Op[name=%s,type=%s]: the input size is %u", op_name.c_str(), op_type.c_str(), input_size);

  // select data type by different precison mode
  Status ret = op_format_dtype_strategy_manager_ptr_->GetAllPossibleDtypeIndexByPrecisionMode(
      prio_index_map, input_map, node_ptr, op_kernel_info_ptr, true, matched_index_vec);
  FE_LOGD("Op[name=%s,type=%s]: After matching dtype, size of matched vec is %zu.", op_name.c_str(), op_type.c_str(),
          matched_index_vec.size());
  return ret;
}

Status OpFormatDtypeJudge::GetInputFormatIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                               const IndexNameMap &input_map,
                                               const std::map<uint32_t, int> &prio_index_map,
                                               vector<uint32_t> &matched_index_vec) {
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  std::string op_name = op_desc_ptr->GetName();
  std::string op_type = op_desc_ptr->GetType();
  uint32_t input_size = op_desc_ptr->GetInputsSize();
  FE_LOGD("Op[name=%s,type=%s]: match origin format for input, input size is %u.", op_name.c_str(), op_type.c_str(),
          input_size);
  Status ret = op_format_dtype_strategy_manager_ptr_->GetAllPossibleFormatIndexByDefaultMode(
      prio_index_map, input_map, node_ptr, op_kernel_info_ptr, true, matched_index_vec);
  FE_LOGD("Op[name=%s,type=%s]: After matching input format, size of matched vec is %zu.",
      op_name.c_str(), op_type.c_str(), matched_index_vec.size());
  return ret;
}

Status OpFormatDtypeJudge::GetOutputFormatIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                                const IndexNameMap &output_map, vector<uint32_t> &matched_index_vec) {
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  uint32_t output_size = op_desc_ptr->GetOutputsSize();
  FE_LOGD("Op[name=%s,type=%s]: match origin format for output, output size is %u.", op_name.c_str(), op_type.c_str(),
          output_size);
  /* For output the sequence of tensor index is just the same as the original
   * sequence */
  std::map<uint32_t, int> prio_index_map;
  for (size_t i = 0; i < op_desc_ptr->GetOutputsSize(); ++i) {
    prio_index_map.emplace(std::make_pair(i, static_cast<int>(i)));
  }
  Status ret = op_format_dtype_strategy_manager_ptr_->GetAllPossibleFormatIndexByDefaultMode(
      prio_index_map, output_map, node_ptr, op_kernel_info_ptr, false, matched_index_vec);
  FE_LOGD("Op[name=%s,type=%s]: After matching output format, size of matched vec is %zu.",
      op_name.c_str(), op_type.c_str(), matched_index_vec.size());
  return ret;
}

Status OpFormatDtypeJudge::GetOutputDtypeIndex(const ge::NodePtr &node_ptr, const OpKernelInfoPtr &op_kernel_info_ptr,
                                               const IndexNameMap &output_map, vector<uint32_t> &matched_index_vec) {
  ge::OpDescPtr op_desc_ptr = node_ptr->GetOpDesc();
  string op_name = op_desc_ptr->GetName();
  string op_type = op_desc_ptr->GetType();
  uint32_t output_size = op_desc_ptr->GetOutputsSize();
  FE_LOGD("Op[name=%s,type=%s]: the output size is %u", op_name.c_str(), op_type.c_str(), output_size);
  /* For output the sequence of tensor index is just the same as the original
   * sequence */
  std::map<uint32_t, int> prio_index_map;
  for (size_t i = 0; i < op_desc_ptr->GetOutputsSize(); ++i) {
    prio_index_map.emplace(std::make_pair(i, static_cast<int>(i)));
  }
  // select data type by different precison mode
  Status ret = op_format_dtype_strategy_manager_ptr_->GetAllPossibleDtypeIndexByPrecisionMode(
      prio_index_map, output_map, node_ptr, op_kernel_info_ptr, false, matched_index_vec);
  FE_LOGD("Op[name=%s,type=%s]: After matching dtype, size of matched vec is %zu.", op_name.c_str(), op_type.c_str(),
          matched_index_vec.size());
  return ret;
}

bool OpFormatDtypeJudge::IsNoNeedJudge(const ge::OpDescPtr &op_desc_ptr, int &imply_type) const {
  string op_type = op_desc_ptr->GetType();
  bool is_special_op = (FeGraphUtils::IsMainGraphData(op_desc_ptr) || FeGraphUtils::IsMainGraphNetOutput(op_desc_ptr) ||
                        op_type == CONSTANT || op_type == CONSTANTOP || op_type == VARIABLE || op_type == TRANSDATA ||
                        op_type == AIPPDATA || CheckVirtualOp(op_desc_ptr));
  bool has_fe_imply_type = ge::AttrUtils::GetInt(op_desc_ptr, FE_IMPLY_TYPE, imply_type);
  return is_special_op || !has_fe_imply_type;
}
}  // namespace fe