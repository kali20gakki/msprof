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

#include "tbe_sgt_json_parse.h"
#include <fstream>
#include <memory>
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/util/json_util.h"
#include "graph/op_kernel_bin.h"
#include "tensor_engine/fusion_api.h"

using nlohmann::json;

namespace fe {
/*
*  @ingroup fe
*  @brief  parse the block_dim info in handle
*  @param   [in] handle
*  @param   [out] op_desc_ set block_dim according to the block_dim info in handle
*  @return SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::ParseTvmBlockDim() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmBlockDim] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<int64_t> block_dim_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    int32_t block_dim;
    if (json_parser.ParseTvmBlockDim(block_dim) == FAILED) {
      FE_LOGE("get attr block_dim for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }
    block_dim_vec.emplace_back(block_dim);
    FE_LOGD("ParseSgtTvmBlockDim: %u %s", block_dim, op_desc_->GetName().c_str());
  }
  (void)ge::AttrUtils::SetListInt(op_desc_, ge::TVM_ATTR_NAME_THREAD_BLOCKDIM, block_dim_vec);

  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseOpKBHitrate() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseOpKBHitrate] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  string graph_op_name;
  string session_graph_id;
  ge::ComputeGraphPtr graph = node_.GetOwnerComputeGraph();
  if (graph) {
    (void)ge::AttrUtils::GetStr(graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    graph_op_name = session_graph_id;
    graph_op_name += "_";
    graph_op_name += op_desc_->GetName();
  } else {
    graph_op_name = op_desc_->GetName();
  }

  if (json_parser_vec_[0].empty()) {
    FE_LOGW("json_parser_vec_ is empty, node[%s].", op_desc_->GetName().c_str());
    return SUCCESS;
  }
  vector<std::string> kernel_name_vec;
  (void)ge::AttrUtils::GetListStr(op_desc_,
                                  json_parser_vec_[0][0].GetAttrPrefix() + kThreadKernelName,
                                  kernel_name_vec);
  if (kernel_name_vec.size() != json_parser_vec_[0].size()) {
      REPORT_FE_ERROR("Size of kernel_name_vec for node[%s] is invalid.", op_desc_->GetName().c_str());
      return FAILED;
  }
  size_t idx = 0UL;

  for (auto &json_parser : json_parser_vec_[0]) {
    int32_t op_hitrate;
    if (json_parser.ParseOpKBHitrate(op_hitrate) == FAILED) {
      FE_LOGE("get attr op_hitrate for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    FE_LOGI("[op_kb_hit][%s][%d][%s]", graph_op_name.c_str(), op_hitrate, kernel_name_vec[idx].c_str());
    ++idx;
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the batch_bind_only info in handle
*  @param   [in] handle
*  @param   [out] op_desc_ set batch_bind_only according to
*   the batch_bind_only info in handle
*  @return SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::ParseBatchBindOnly() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseBatchBindOnly] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<bool> batch_bind_only_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    uint32_t batch_bind_only;
    if (json_parser.ParseBatchBindOnly(batch_bind_only) == FAILED) {
      FE_LOGE("get attr _is_n_batch_split for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    batch_bind_only_vec.push_back(static_cast<bool>(batch_bind_only));
    FE_LOGD("Parse batch_bind_only[%u] from node [%s].", batch_bind_only, op_desc_->GetName().c_str());
  }

  if (!ge::AttrUtils::SetListBool(op_desc_, ge::TVM_ATTR_NAME_THREAD_N_BATCH_SPLIT, batch_bind_only_vec)) {
    FE_LOGE("Set attr _is_n_batch_split for node[%s] failed.", op_desc_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief parse the magic info in handle
*  @param   [in] handle
*  @param   [out] op_desc_ set magic according to magic info in handle
*  @return SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::ParseTvmMagic() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmMagic] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  vector<std::string> magic_vec;
  vector<std::string> core_type;
  int32_t index = 0;
  for (auto &json_parser : json_parser_vec_[0]) {
    FE_LOGD("Parse sgt json %d.", index);
    std::string magic;
    if (json_parser.ParseTvmMagic(magic) == FAILED) {
      FE_LOGE("get attr magic for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    auto iter = std::find(kBinaryMagicTypesVec.begin(), kBinaryMagicTypesVec.end(), magic);
    if (iter == kBinaryMagicTypesVec.end()) {
      FE_LOGE("The magic value[%s] is not valid.", magic.c_str());
      FE_LOGE("[SubGraphOpt][ParseSgtJson][ParseTvmMgc] Only support \
              RT_DEV_BINARY_MAGIC_(ELF_AICPU/ELF/ELF_AIVEC/ELF_AICUBE/ELF_MIX_AIC/ELF_MIX_AIV).");
      return FAILED;
    }
    if (magic == "FFTS_BINARY_MAGIC_ELF_MIX_AIC") {
      core_type.emplace_back("MIX_AIC");
    } else if (magic == "FFTS_BINARY_MAGIC_ELF_MIX_AIV") {
      core_type.emplace_back("MIX_AIV");
    }
    magic_vec.emplace_back(magic);
  }
  (void) ge::AttrUtils::SetListStr(op_desc_, ge::TVM_ATTR_NAME_THREAD_MAGIC, magic_vec);

  if (!core_type.empty()) {
    (void) ge::AttrUtils::SetListStr(op_desc_, ATTR_NAME_THREAD_CUBE_VECTOR_CORE_TYPE, core_type);
  }

  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseTvmCoreType() {
  vector<vector<std::string>> core_types;
  for (auto &core_json_parser : json_parser_vec_) {
    vector<std::string> core_type_vec;
    for (auto &json_parser : core_json_parser) {
      std::string core_type;
      if (json_parser.ParseTvmCoreType(core_type) == FAILED) {
        FE_LOGE("get attr core type for node[%s] failed.", op_desc_->GetName().c_str());
        return FAILED;
      }

      if (core_type.empty()) {
        return SUCCESS;
      }

      if ((core_type != "AIC" && core_type != "AIV")) {
        FE_LOGE("Data error! 'core_type' should be AIC or AIV.");
        return FAILED;
      }
      core_type_vec.emplace_back(core_type);
    }
    core_types.push_back(core_type_vec);
  }
  (void)ge::AttrUtils::SetListStr(op_desc_, ATTR_NAME_THREAD_CUBE_VECTOR_CORE_TYPE, core_types[0]);
  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseTvmTaskRatio() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmTaskRatio] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  vector<uint32_t> task_ratios;
  for (auto &json_parser : json_parser_vec_[0]) {
    uint32_t task_ratio = 0;
    if (json_parser.ParseTvmTaskRatio(task_ratio) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmTaskRatio] get task_ratio for node[%s] failed.",
                      op_desc_->GetName().c_str());
      return FAILED;
    }
    if (task_ratio == 0) {
      FE_LOGD("Node[%s] task ratio is empty.", op_desc_->GetName().c_str());
      return SUCCESS;
    }
    FE_LOGD("Parse task_ratio[%u] from node [%s].", task_ratio, op_desc_->GetName().c_str());
    task_ratios.push_back(task_ratio);
  }
  if (!ge::AttrUtils::SetListInt(op_desc_, kThreadTaskRadio, task_ratios)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmTaskRatio] Set attr task_ratio for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseTvmModeInArgsFirstField() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmModeInArgsFirstField] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<bool> mode_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    uint32_t mode;
    if (json_parser.ParseModeInArgsFirstField(mode) == FAILED) {
      FE_LOGE("get attr modeInArgsFirstField for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    mode_vec.push_back(static_cast<bool>(mode));
    FE_LOGD("Parse modeInArgsFirstField[%u] from node [%s].", mode, op_desc_->GetName().c_str());
  }

  if (!ge::AttrUtils::SetListBool(op_desc_, kThreadModeInArgsFirstField, mode_vec)) {
    FE_LOGE("Set attr modeInArgsFirstField for node[%s] failed.", op_desc_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

/* OpKernelBinPtr
 *  @ingroup fe
 *  @brief  parse the workspace info in handle
 *  @param   [in] handle
 *  @param   [out] op_desc_, tvm_workspace_sizes_  set workspace according to
 * block_dim info in handle
 *  @return SUCCESS or FAILED
 */
Status TbeSgtJsonFileParse::ParseTvmWorkSpace() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmWorkSpace] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  /* The following vector contains non-tail and tail workspace information. */
  std::vector<std::vector<int64_t>> tvm_workspace_types_vec;
  std::vector<std::vector<int64_t>> tvm_workspace_sizes_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    std::vector<int64_t> tvm_workspace_types;
    std::vector<int64_t> tvm_workspace_sizes;
    if (json_parser.ParseTvmWorkSpace(tvm_workspace_sizes, tvm_workspace_types) == FAILED) {
      FE_LOGE("get attr workspace for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }
    bool is_type_and_size_empty = tvm_workspace_types.empty() && tvm_workspace_sizes.empty();
    if (is_type_and_size_empty) {
      return SUCCESS;
    }

    tvm_workspace_sizes_vec.emplace_back(tvm_workspace_sizes);
    tvm_workspace_types_vec.emplace_back(tvm_workspace_types);
  }

  uint32_t thread_num = slice_info_ptr_->slice_instance_num;
  /* The size of one non-tail workspace.
   * Attention!! This variable is not the total number of all non-tail workspaces. */
  size_t non_tail_workspace_size = 0;
  int branch1 = (thread_num == 1 && tvm_workspace_sizes_vec.size() == 1);
  int branch2 = (thread_num >= kTotalSgtJsonNum && tvm_workspace_types_vec.size() == kTotalSgtJsonNum);
  if (thread_num == 0) {
    FE_LOGE("[SubGraphOpt][Compile][ParseSgtWorkSpace][Node %s, type %s]Thread num is zero.",
            op_desc_->GetName().c_str(), op_desc_->GetType().c_str());
    return FAILED;

  } else if (branch1) {
    non_tail_workspace_size =  tvm_workspace_sizes_vec[0].size();
  } else if (branch2) {
    /* We combine all non-tail threads' workspace size together and
     * then pad the tail thread's workspace at the end of this vector. */
    non_tail_workspace_size = tvm_workspace_sizes_vec[0].size();
    for (auto &ele : tvm_workspace_sizes_vec[0]) {
      ele = ele * (int32_t)(thread_num - 1);
    }
    /* Pad the tail thread's workspace at the end. */
    for (auto &ele : tvm_workspace_sizes_vec[1]) {
      tvm_workspace_sizes_vec[0].emplace_back(ele);
    }

    for (auto &ele : tvm_workspace_types_vec[1]) {
      tvm_workspace_types_vec[0].emplace_back(ele);
    }

  } else {
    FE_LOGE("[SubGraphOpt][Compile][ParseSgtWorkSpace][Node %s, type %s]Thread num %u or "
            "workspace num [%zu, %zu] is invalid.",
            op_desc_->GetName().c_str(), op_desc_->GetType().c_str(),
            thread_num, tvm_workspace_sizes_vec.size(), tvm_workspace_types_vec.size());
    return FAILED;
  }
  bool is_type_or_size_empty = tvm_workspace_sizes_vec.empty() || tvm_workspace_types_vec.empty();
  if (is_type_or_size_empty) {
    return FAILED;
  }
  op_desc_->SetWorkspaceBytes(tvm_workspace_sizes_vec[0]);
  (void)ge::AttrUtils::SetInt(op_desc_, NON_TAIL_WORKSPACE_SIZE, non_tail_workspace_size);
  if (!tvm_workspace_types_vec[0].empty()) {
    ge::AttrUtils::SetListInt(op_desc_, ge::TVM_ATTR_NAME_WORKSPACE_TYPE, tvm_workspace_types_vec[0]);
  }
  return SUCCESS;
}

void TbeSgtJsonFileParse::GetWorkspaceAtomicFlagAndOutputIndexFlag(const std::vector<int64_t> &parameters_index,
                                                                   const size_t &workspace_num,
                                                                   const size_t &input_num, const size_t &output_num,
                                                                   std::vector<int64_t> &output_index,
                                                                   int64_t &workspace_atomic_flag,
                                                                   bool &output_index_flag) {
  size_t parameters_index_size = parameters_index.size();
  for (size_t i = 0; i < workspace_num; ++i) {
    size_t index = input_num + i;
    if (index >= parameters_index_size) {
      continue;
    }
    if (parameters_index[index] == 1) {
      workspace_atomic_flag = 1;
      break;
    }
  }
  for (size_t i = 0; i < output_num; ++i) {
    size_t index = input_num + workspace_num + i;
    if (index >= parameters_index_size) {
      continue;
    }
    output_index.push_back(parameters_index[index]);
    if (parameters_index[index] != 0) {
      output_index_flag = true;
    }
  }
}

Status TbeSgtJsonFileParse::SetAtomicInfo(std::vector<int64_t> &parameters_index,
                                          std::vector<int64_t> &ub_atomic_params,
                                          int64_t &workspace_atomic_flag,
                                          std::vector<int64_t> &output_index) {
  std::vector<int64_t> cur_output_index;
  size_t input_num = op_desc_->GetInputsSize();
  size_t workspace_num = op_desc_->GetWorkspaceBytes().size();
  size_t output_num = op_desc_->GetOutputsSize();
  size_t total_size = input_num + workspace_num + output_num;
  bool is_support_unknown_shape = false;
  if (ge::AttrUtils::GetBool(op_desc_, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, is_support_unknown_shape) &&
      is_support_unknown_shape) {
    total_size = input_num + workspace_num + output_num + 1;
  }
  if (total_size >= parameters_index.size()) {
    size_t loss_num = total_size - parameters_index.size();
    for (size_t i = 0; i < loss_num; ++i) {
      parameters_index.push_back(0);
    }
    ub_atomic_params = parameters_index;
  } else {
    FE_LOGD("parameters size is larger than input&workspace&output.");
    FE_LOGD("inputNum:%zu, output_num:%zu, workspaceSize:%zu, para_size:%zu name:%s", input_num, output_num,
            workspace_num, parameters_index.size(), op_desc_->GetName().c_str());
    ub_atomic_params = parameters_index;
    return SUCCESS;
  }
  // in parameters data sort as input->workspace->output
  bool output_index_flag = false;
  GetWorkspaceAtomicFlagAndOutputIndexFlag(parameters_index, workspace_num, input_num, output_num,
                                           cur_output_index, workspace_atomic_flag, output_index_flag);

  if (output_index_flag) {
    output_index = cur_output_index;
  }
  return SUCCESS;
}
/*
*  @ingroup fe
*  @brief  parse the parameters info in handle
*  @param   [in] handle
*  @param   [out] op_desc_, set output_index and workspace
*  according to parameters info in handle
*  @return SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::ParseTvmParameters() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmParameters] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<std::vector<int64_t>> atomic_ouput_index_vec;
  std::vector<std::vector<int64_t>> ub_atomic_params_vec;
  std::vector<int64_t> atomic_workspace_flag_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    std::vector<int64_t> parameters_index;
    std::vector<int64_t> ub_atomic_params;
    int64_t workspace_atomic_flag = -1;
    std::vector<int64_t> output_index;
    if (json_parser.ParseTvmParameters(parameters_index) == FAILED) {
      FE_LOGE("get attr parameters for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    if (parameters_index.empty()) {
      return SUCCESS;
    }
    if (SetAtomicInfo(parameters_index, ub_atomic_params, workspace_atomic_flag, output_index) != SUCCESS) {
      FE_LOGE("set atomic info for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    ub_atomic_params_vec.push_back(ub_atomic_params);
    if (workspace_atomic_flag != -1) {
      atomic_workspace_flag_vec.push_back(workspace_atomic_flag);
      if (!output_index.empty()) {
        atomic_ouput_index_vec.push_back(output_index);
      }
    }
  }

  (void)ge::AttrUtils::SetListListInt(op_desc_, "thread_ub_atomic_params", ub_atomic_params_vec);
  if (!atomic_workspace_flag_vec.empty()) {
    (void)ge::AttrUtils::SetListInt(op_desc_, TBE_OP_THREAD_ATOMIC_WORKSPACE_FLAG, atomic_workspace_flag_vec);

    if (!atomic_ouput_index_vec.empty()) {
      (void)ge::AttrUtils::SetListListInt(op_desc_, TBE_OP_THREAD_ATOMIC_OUTPUT_INDEX, atomic_ouput_index_vec);
    }
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the meta_data info in handle
*  @param   [in] handle
*  @param   [out] op_desc_, set meta_data according to meta_data info in handle
*  @return SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::ParseTvmMetaData() {
  for (auto &core_json_parser : json_parser_vec_) {
    std::vector<std::string> meta_data_vec;
    for (auto &json_parser : core_json_parser) {
      string meta_data;
      if (json_parser.ParseTvmMetaData(meta_data) == FAILED) {
        FE_LOGE("get attr metadata for node[%s] failed.", op_desc_->GetName().c_str());
        return FAILED;
      }

      meta_data_vec.push_back(meta_data);
    }
    (void)ge::AttrUtils::SetListStr(op_desc_,
        core_json_parser[0].GetAttrPrefix() + ge::TVM_ATTR_NAME_THREAD_METADATA, meta_data_vec);
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the kernel_name info in handle
*  @param   [in] handle
*  @param   [out] op_desc_, add kernel_name to op_desc_.name according to
* kernel_name info in handle
*  @return SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::ParseTvmKernelName() {
  for (auto &core_json_parser : json_parser_vec_) {
    std::vector<std::string> kernel_name_vec;
    for (auto &json_parser : core_json_parser) {
      std::string kernel_name;
      if (json_parser.ParseTvmKernelName(kernel_name) == FAILED) {
        REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmKernelName] get attr kernel name for node[%s] "
                        "failed.", op_desc_->GetName().c_str());
        return FAILED;
      }
      kernel_name_vec.push_back(kernel_name);
    }
    (void)ge::AttrUtils::SetListStr(op_desc_,
                                    core_json_parser[0].GetAttrPrefix() + kThreadKernelName,
                                    kernel_name_vec);
  }
  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseGlobleWorkspaceStatus() {
  ge::ComputeGraphPtr graph = node_.GetOwnerComputeGraph();
  if (graph == nullptr) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseGlobleWorkspaceStatus] get graph for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("[SubGraphOpt][ParseSgtJson][ParseGlobleWorkspaceStatus] get graph[%s] success for node[%s].",
          graph->GetName().c_str(), node_.GetName().c_str());
  for (auto &core_json_parser : json_parser_vec_) {
    for (auto &json_parser : core_json_parser) {
      if (json_parser.ParseGlobleWorkspaceStatus(graph, op_desc_) == FAILED) {
        REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmKernelName] get globleworkspace_status for node[%s] "
                        "failed.", op_desc_->GetName().c_str());
        return FAILED;
      }
    }
  }
  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseConvCompressParameters() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseConvCompressParameters] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<std::vector<int64_t>> compress_params_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    std::vector<int64_t> compress_params;
    if (json_parser.ParseConvCompressParameters(compress_params) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmKernelName] get attr compress parameters "
                      "for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    if (compress_params.empty()) {
      return SUCCESS;
    }
    compress_params_vec.push_back(compress_params);
  }
  if (!ge::AttrUtils::SetListListInt(op_desc_, ATTR_NAME_THREAD_COMPRESS_PARAMETERS, compress_params_vec)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmKernelName] Fail to set attr compress_weight "
                    "for node[%s].", op_desc_->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseWeightRepeat() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseWeightRepeat] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<int64_t> weight_repeat_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    int64_t weight_repeat = INT64_MAX;
    if (json_parser.ParseWeightRepeat(weight_repeat)) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmKernelName] get attr weight repeat "
                      "for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }

    if (weight_repeat == INT64_MAX) {
      return SUCCESS;
    }

    weight_repeat_vec.push_back(weight_repeat);
    FE_LOGI("The weight repeat of node[%s] is %ld", op_desc_->GetName().c_str(), weight_repeat);
  }

  if (!ge::AttrUtils::SetListInt(op_desc_, ATTR_NAME_THREAD_WEIGHT_REPEAT, weight_repeat_vec)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmKernelName] Fail to set attr weight_repeat "
                    "for node[%s].", op_desc_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status TbeSgtJsonFileParse::ParseOpParaSize() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseOpParaSize] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<int64_t> op_para_size_vec;
  for (auto &json_parser : json_parser_vec_[0]) {
    int64_t op_para_size;
    if (json_parser.ParseOpParaSize(op_para_size) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][ParseTvmKernelName] get attr op para size "
                      "for node[%s] failed.", op_desc_->GetName().c_str());
      return FAILED;
    }
    FE_LOGD("ParseOpParaSize: %ld %s", op_para_size, op_desc_->GetName().c_str());
  }
  (void)ge::AttrUtils::SetListInt(op_desc_, OP_THREAD_PARA_SIZE, op_para_size_vec);

  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  joint the path of bin file, if success, renew the op_desc.name
*  @param   [in] handle
*  @param   [out] op_desc_->name
*  @return  SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::PackageTvmBinFile() {
  std::vector<ge::OpKernelBinPtr> kernel_bin_ptr_vec;
  std::vector<std::string> tbe_kernel_name_vec;
  std::vector<ge::Buffer> buffer_vec;
  std::vector<int32_t> tbe_kernel_size;

  for (size_t i = 0; i < json_parser_vec_.size(); i++) {
    if (json_parser_vec_[i].empty()) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseSgtJson][PackageTvmBinFile] json parser is empty for node[%s].",
                      op_desc_->GetName().c_str());
      return FAILED;
    }
    for (size_t j = 0; j < json_parser_vec_[i].size(); j++) {
      vector<char> buffer;
      if (i < tvm_file_path_vec_.size() && j < tvm_file_path_vec_[i].size()) {
        op_desc_->SetExtAttr(ge::ATTR_NAME_OP_FILE_PATH, tvm_file_path_vec_[i][j]);
        if (json_parser_vec_[i][j].PackageTvmBinFile(tvm_file_path_vec_[i][j], buffer) == FAILED) {
          FE_LOGE("get attr bin file for node[%s] failed.", op_desc_->GetName().c_str());
          return FAILED;
        }
      }

      std::string kernel_name;
      ge::AttrUtils::GetStr(op_desc_, op_desc_->GetName() + "_kernelname", kernel_name);
      ge::OpKernelBinPtr tbe_kernel_ptr;
      FE_MAKE_SHARED(tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(kernel_name, std::move(buffer)), return FAILED);
      kernel_bin_ptr_vec.push_back(tbe_kernel_ptr);
      tbe_kernel_name_vec.push_back(tbe_kernel_ptr->GetName());

      ge::Buffer tbe_kernel_buffer(tbe_kernel_ptr->GetBinDataSize());
      tbe_kernel_buffer = ge::Buffer::CopyFrom(tbe_kernel_ptr->GetBinData(), tbe_kernel_ptr->GetBinDataSize());
      buffer_vec.push_back(tbe_kernel_buffer);
      tbe_kernel_size.push_back(static_cast<int32_t>(tbe_kernel_ptr->GetBinDataSize()));
    }

    (void)op_desc_->SetExtAttr(json_parser_vec_[i][0].GetAttrPrefix() + ge::OP_EXTATTR_NAME_THREAD_TBE_KERNEL,
                         kernel_bin_ptr_vec);
    (void)ge::AttrUtils::SetListStr(op_desc_,
        json_parser_vec_[i][0].GetAttrPrefix() + ge::ATTR_NAME_THREAD_TBE_KERNEL_NAME, tbe_kernel_name_vec);
    (void)ge::AttrUtils::SetListBytes(op_desc_,
        json_parser_vec_[i][0].GetAttrPrefix() + ge::ATTR_NAME_THREAD_TBE_KERNEL_BUFFER, buffer_vec);
    (void)ge::AttrUtils::SetListInt(op_desc_,
        json_parser_vec_[i][0].GetAttrPrefix() + ATTR_NAME_THREAD_TBE_KERNEL_SIZE, tbe_kernel_size);
  }
  return SUCCESS;
}
void TbeSgtJsonFileParse::PackageTvmJsonInfoSub(size_t &core_type_num, vector<std::string> &bin_file_path_vec) {
  for (size_t i = 0; i < core_type_num; i++) {
    vector<TbeJsonFileParseImpl> thread_json_parser_vec;
    vector<std::string> thread_tvm_file_vec;
    for (size_t j = 0; j < kThreadNumMax; j++) {
      TbeJsonFileParseImpl json_parser;
      thread_json_parser_vec.push_back(json_parser);
      thread_tvm_file_vec.emplace_back("");
    }
    json_parser_vec_.push_back(thread_json_parser_vec);
    tvm_file_path_vec_.push_back(thread_tvm_file_vec);
  }
  for (size_t i = 0; i < core_type_num; i++) {
    for (size_t j = 0; j < kThreadNumMax; j++) {
      if (i + j * core_type_num >= bin_file_path_vec.size()) {
        continue;
      }
      json_parser_vec_[i][j].Initialize(bin_file_path_vec[i + j * core_type_num]);
      tvm_file_path_vec_[i][j] = ".";
      size_t find_pos = bin_file_path_vec[i + j * core_type_num].find_last_of("\\/");
      if (find_pos != std::string::npos) {
        tvm_file_path_vec_[i][j] = bin_file_path_vec[i + j * core_type_num].substr(0, find_pos);
      }
    }
  }
}
/*
*  @ingroup fe
*  @brief   package the json info together
*  @param   [in]  info
*  @param   [out] tvm_file_path_
*  @return  SUCCESS or FAILED
*/
Status TbeSgtJsonFileParse::PackageTvmJsonInfo(const vector<std::string> &json_file_path_vec,
                                               vector<std::string> &bin_file_path_vec) {
  FE_LOGW("size of json file path is %zu",json_file_path_vec.size());
  vector<std::string> all_json_file_path_vec;
  for (auto json_file : json_file_path_vec) {
    if (RealPath(json_file).empty()) {
      std::string mix_aic_json = GetSuffixJsonFile(json_file, "_mix_aic");
      std::string mix_aiv_json = GetSuffixJsonFile(json_file, "_mix_aiv");
      if (!RealPath(mix_aic_json).empty() && !RealPath(mix_aiv_json).empty()) {
        all_json_file_path_vec.push_back(mix_aic_json);
        all_json_file_path_vec.push_back(mix_aiv_json);
      } else {
        REPORT_FE_ERROR("[SubGraphOpt][ParseJson][PkgTvmJsInfo] ReadJsonFile failed.");
        return FAILED;
      }
    } else {
      all_json_file_path_vec.push_back(json_file);
    }
  }
  bin_file_path_vec = all_json_file_path_vec;

  size_t json_file_size = all_json_file_path_vec.size();
  size_t core_type_num = json_file_size / kThreadNumMax;
  if (core_type_num > kThreadJsonNumMax || json_file_size % kThreadNumMax != 0) {
    FE_LOGE("Node[%s] json file num is not correct.", op_desc_->GetName().c_str());
    return FAILED;
  }
  PackageTvmJsonInfoSub(core_type_num, bin_file_path_vec);

  slice_info_ptr_ = op_desc_->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr_);

  for (auto &parseFunc : parse_func_map) {
    if ((this->*(parseFunc.second))() != SUCCESS) {
      FE_LOGE("Parse failed, the json file:%s.", parseFunc.first.c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}
}  // namespace fe

