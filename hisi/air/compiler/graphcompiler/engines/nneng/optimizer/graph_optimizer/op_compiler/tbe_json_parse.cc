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

#include "graph_optimizer/op_compiler/tbe_json_parse.h"
#include <fstream>
#include <memory>
#include "common/fe_log.h"
#include "common/fe_utils.h"
#include "common/configuration.h"
#include "common/util/json_util.h"
#include "common/aicore_util_attr_define.h"
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
Status TbeJsonFileParse::ParseTvmBlockDim() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmBlkDim] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  int32_t block_dim;
  if (json_parser_vec_[0].ParseTvmBlockDim(block_dim) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmBlkDim] get the blockDim for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  (void)ge::AttrUtils::SetInt(op_desc_, ge::TVM_ATTR_NAME_BLOCKDIM, block_dim);
  FE_LOGD("ParseTvmBlockDim: %u %s", block_dim, op_desc_->GetName().c_str());
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief  parse the batch_bind_only info in handle
*  @param   [in] handle
*  @param   [out] op_desc_ set batch_bind_only according to
*  the batch_bind_only info in handle
*  @return SUCCESS or FAILED
*/

Status TbeJsonFileParse::ParseBatchBindOnly() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseBatchBindOnly] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  uint32_t batch_bind_only;
  if (json_parser_vec_[0].ParseBatchBindOnly(batch_bind_only) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseBtcBindOnly] get the batch_bind_only for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  FE_LOGD("Parse batch_bind_only[%u] from node [%s].", batch_bind_only, op_desc_->GetName().c_str());
  if (!ge::AttrUtils::SetBool(op_desc_, ge::ATTR_N_BATCH_SPILT, batch_bind_only)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseBtcBindOnly] Set attr _is_n_batch_split for node[%s] failed.",
                    op_desc_->GetName().c_str());
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
Status TbeJsonFileParse::ParseTvmMagic() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmMagic] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::string magic;
  if (json_parser_vec_[0].ParseTvmMagic(magic) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmMgc] get the magic for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  auto iter = std::find(kBinaryMagicTypesVec.begin(), kBinaryMagicTypesVec.end(), magic);
  if (iter == kBinaryMagicTypesVec.end()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmMgc] The magic value[%s] is not valid.", magic.c_str());
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmMgc] Only support \
                    RT_DEV_BINARY_MAGIC_(ELF_AICPU/ELF/ELF_AIVEC/ELF_AICUBE/ELF_MIX_AIC/ELF_MIX_AIV).");
    return FAILED;
  }
  bool is_mix = false;
  // accord to magic, judge cube type
  if (magic == "FFTS_BINARY_MAGIC_ELF_MIX_AIC") {
    is_mix = true;
    (void) ge::AttrUtils::SetStr(op_desc_, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIC");
  } else if (magic == "FFTS_BINARY_MAGIC_ELF_MIX_AIV") {
    is_mix = true;
    (void) ge::AttrUtils::SetStr(op_desc_, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, "MIX_AIV");
  }
  bool ffts_node = false;
  (void)ge::AttrUtils::GetBool(op_desc_, kTypeFFTSPlus, ffts_node);
  if (is_mix && !ffts_node && fe::Configuration::Instance(fe::AI_CORE_NAME).GetMixL2Enable()) {
    (void)ge::AttrUtils::SetStr(op_desc_, ATTR_NAME_ALIAS_ENGINE_NAME, "ffts_plus");
  }
  (void)ge::AttrUtils::SetStr(op_desc_, ge::TVM_ATTR_NAME_MAGIC, magic);

  return SUCCESS;
}

Status TbeJsonFileParse::ParseTvmCoreType() {
  vector<std::string> core_types;
  for (auto &json_parser_ : json_parser_vec_) {
    std::string core_type;
    if (json_parser_.ParseTvmCoreType(core_type) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmCoreType] get the core_type for node[%s] failed.",
                      op_desc_->GetName().c_str());
      return FAILED;
    }

  if (core_type.empty()) {
    return SUCCESS;
  }

  if ((core_type != "AIC" && core_type != "AIV")) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmCoreType] Data error! 'core_type' should be AIC or AIV. \
                    Current core type is %s.", core_type.c_str());
    return FAILED;
  }
    core_types.push_back(core_type);
  }
  (void)ge::AttrUtils::SetStr(op_desc_, ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_types[0]);
  return SUCCESS;
}

Status TbeJsonFileParse::ParseTvmTaskRatio() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmTaskRatio] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  uint32_t task_ratio = 0;
  if (json_parser_vec_[0].ParseTvmTaskRatio(task_ratio) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmTaskRatio] get task_ratio for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  if (task_ratio == 0) {
    FE_LOGD("Node[%s] task ratio is empty.", op_desc_->GetName().c_str());
    return SUCCESS;
  }
  FE_LOGD("Parse task_ratio[%u] from node [%s].", task_ratio, op_desc_->GetName().c_str());
  if (!ge::AttrUtils::SetInt(op_desc_, kTaskRadio, task_ratio)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmTaskRatio] Set attr task_ratio for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}

Status TbeJsonFileParse::ParseTvmModeInArgsFirstField() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmModeInArgs] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  uint32_t mode = 0;
  if (json_parser_vec_[0].ParseModeInArgsFirstField(mode) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmModeInArgs] get modeInArgsFirstField for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  FE_LOGD("Parse modeInArgsFirstField[%u] from node [%s].", mode, op_desc_->GetName().c_str());
  if (!ge::AttrUtils::SetInt(op_desc_, kModeInArgsFirstField, mode)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmModeInArgs] Set attr modeInArgsFirstField for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}
Status TbeJsonFileParse::ParseTvmKernelList() {
  for (auto json_parser_ : json_parser_vec_) {
    std::string kernel_list_first;
    if (json_parser_.ParseTvmKernelList(kernel_list_first) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmKernList] ParseTvmKernelList get kernelList for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  FE_LOGI("kernel list first node name is %s.", kernel_list_first.c_str());
  if (!kernel_list_first.empty()) {
      (void)ge::AttrUtils::SetStr(op_desc_,
                            json_parser_.GetAttrPrefix() + ATTR_NAME_KERNEL_LIST_FIRST_NAME, kernel_list_first);
    }
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
Status TbeJsonFileParse::ParseTvmWorkSpace() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmWorkSpace] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<int64_t> tvm_workspace_types;
  std::vector<int64_t> tvm_workspace_sizes;
  if (json_parser_vec_[0].ParseTvmWorkSpace(tvm_workspace_sizes, tvm_workspace_types) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmWorkSpace] get the workspace for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  FE_LOGI("work space size for node %s is %zu", op_desc_->GetName().c_str(), tvm_workspace_sizes.size());
  if (!tvm_workspace_sizes.empty()) {
    op_desc_->SetWorkspaceBytes(tvm_workspace_sizes);
  }

  if (!tvm_workspace_types.empty()) {
    (void)ge::AttrUtils::SetListInt(op_desc_, ge::TVM_ATTR_NAME_WORKSPACE_TYPE, tvm_workspace_types);
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
Status TbeJsonFileParse::ParseTvmParameters() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmParameters] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<int64_t> parameters_index;
  if (json_parser_vec_[0].ParseTvmParameters(parameters_index) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmParm] get the parameters for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  if (parameters_index.empty()) {
    return SUCCESS;
  }

  return SetAtomicInfo(parameters_index);
}

void TbeJsonFileParse::GetWorkspaceAtomicFlagAndOutputIndexFlag(const std::vector<int64_t> &parameters_index,
                                                                const size_t &workspace_num,
                                                                const size_t &input_num, const size_t &output_num,
                                                                std::vector<int64_t> &output_index,
                                                                std::vector<int64_t> &workspace_index,
                                                                bool &workspace_atomic_flag,
                                                                bool &output_index_flag) {
  size_t parameters_index_size = parameters_index.size();
  for (size_t i = 0; i < workspace_num; ++i) {
    size_t index = input_num + output_num + i;
    if (index >= parameters_index_size) {
      continue;
    }
    workspace_index.push_back(parameters_index[index]);
    if (parameters_index[index] != 0) {
      workspace_atomic_flag = true;
    }
  }
  for (size_t i = 0; i < output_num; ++i) {
    size_t index = input_num + i;
    if (index >= parameters_index_size) {
      continue;
    }
    output_index.push_back(parameters_index[index]);
    if (parameters_index[index] != 0) {
      output_index_flag = true;
    }
  }
}

Status TbeJsonFileParse::SetAtomicInfo(std::vector<int64_t> &parameters_index) {
  // need modify
  std::vector<int64_t> output_index;
  std::vector<int64_t> workspace_index;
  size_t input_num = op_desc_->GetInputsSize();
  size_t workspace_num = op_desc_->GetWorkspaceBytes().size();
  size_t output_num = op_desc_->GetOutputsSize();
  size_t total_size = input_num + output_num + workspace_num;
  bool is_support_dynamic_shape = false;
  if (ge::AttrUtils::GetBool(op_desc_, ATTR_NAME_SUPPORT_DYNAMIC_SHAPE, is_support_dynamic_shape) &&
      is_support_dynamic_shape) {
    total_size = input_num + workspace_num + output_num + 1;
  }
  if (total_size >= parameters_index.size()) {
    size_t loss_num = total_size - parameters_index.size();
    for (size_t i = 0; i < loss_num; ++i) {
      parameters_index.push_back(0);
    }
  } else {
    FE_LOGD("parameters size is larger than input&output&workspace.");
    FE_LOGD("inputNum:%zu, output_num:%zu, workspaceSize:%zu, para_size:%zu name:%s", input_num, output_num,
            workspace_num, parameters_index.size(), op_desc_->GetName().c_str());
  }
  (void)ge::AttrUtils::SetListInt(op_desc_, "ub_atomic_params", parameters_index);
  // in parameters data sort as input->output->workspace
  bool output_index_flag = false;
  bool workspace_atomic_flag = false;
  GetWorkspaceAtomicFlagAndOutputIndexFlag(parameters_index, workspace_num, input_num, output_num,
                                           output_index, workspace_index, workspace_atomic_flag, output_index_flag);

  (void)ge::AttrUtils::SetInt(op_desc_, TBE_OP_ATOMIC_WORKSPACE_FLAG, static_cast<int>(workspace_atomic_flag));

  if (output_index_flag) {
    (void)ge::AttrUtils::SetListInt(op_desc_, TBE_OP_ATOMIC_OUTPUT_INDEX, output_index);
  }
  if (workspace_atomic_flag) {
    (void)ge::AttrUtils::SetListInt(op_desc_, TBE_OP_ATOMIC_WORKSPACE_INDEX, workspace_index);
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
Status TbeJsonFileParse::ParseTvmMetaData() {
  string meta_data;
  for (auto json_parser_ : json_parser_vec_) {
    if (json_parser_.ParseTvmMetaData(meta_data) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmMetaData] get attr metadata for node[%s] failed.",
                      op_desc_->GetName().c_str());
      return FAILED;
    }

    if (meta_data.empty()) {
      return SUCCESS;
    }

    (void)ge::AttrUtils::SetStr(op_desc_, json_parser_.GetAttrPrefix() + ge::TVM_ATTR_NAME_METADATA, meta_data);
  }
  return SUCCESS;
}

Status TbeJsonFileParse::ParseGlobleWorkspaceStatus() {
  ge::ComputeGraphPtr graph = node_.GetOwnerComputeGraph();
  if (graph == nullptr) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseGlobleWorkspaceStatus] get graph for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  FE_LOGD("[SubGraphOpt][ParseJson][ParseGlobleWorkspaceStatus] get graph[%s] success for node[%s].",
          graph->GetName().c_str(), node_.GetName().c_str());
  for (auto json_parser_ : json_parser_vec_) {
    if (json_parser_.ParseGlobleWorkspaceStatus(graph, op_desc_) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseGlobleWorkspaceStatus] "
                      "get globleworkspace_status for node[%s] failed.",
                      op_desc_->GetName().c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}

Status TbeJsonFileParse::ParseOpKBHitrate() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseOpKBHitrate] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  string graph_node_name;
  string session_graph_id;
  ge::ComputeGraphPtr graph = node_.GetOwnerComputeGraph();
  if (graph) {
    (void)ge::AttrUtils::GetStr(graph, ge::ATTR_NAME_SESSION_GRAPH_ID, session_graph_id);
    graph_node_name = session_graph_id;
    graph_node_name += "_";
    graph_node_name += op_desc_->GetName();
  } else {
    graph_node_name = op_desc_->GetName();
  }

  int32_t op_hitrate = 0;
  if (json_parser_vec_[0].ParseOpKBHitrate(op_hitrate) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseOpKBHitrate] get the op_hitrate for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::string kernel_name;
  (void)ge::AttrUtils::GetStr(op_desc_,
                              json_parser_vec_[0].GetAttrPrefix() + op_desc_->GetName() + "_kernelname", kernel_name);
  FE_LOGI("[op_kb_hit][%s][%d][%s]", graph_node_name.c_str(), op_hitrate, kernel_name.c_str());
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
Status TbeJsonFileParse::ParseTvmKernelName() {
  std::string kernel_name;
  for (auto json_parser_ : json_parser_vec_) {
    if (json_parser_.ParseTvmKernelName(kernel_name) == FAILED) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmKernNm] get attr kernel name for node[%s] failed.",
                      op_desc_->GetName().c_str());
      return FAILED;
    }

    (void)ge::AttrUtils::SetStr(op_desc_,
                                json_parser_.GetAttrPrefix() + op_desc_->GetName() + "_kernelname", kernel_name);
  }
  return SUCCESS;
}

Status TbeJsonFileParse::ParseConvCompressParameters() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseConvCompressParameters] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  std::vector<int64_t> compress_param_vec;
  if (json_parser_vec_[0].ParseConvCompressParameters(compress_param_vec) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmKernNm] get the compress_parameters for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  if (compress_param_vec.empty()) {
    return SUCCESS;
  }

  if (!ge::AttrUtils::SetListInt(op_desc_, ATTR_NAME_COMPRESS_PARAMETERS, compress_param_vec)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseTvmKernNm] Fail to set attr compress_weight for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status TbeJsonFileParse::ParseWeightRepeat() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseWeightRepeat] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  int64_t weight_repeat = INT64_MAX;
  if (json_parser_vec_[0].ParseWeightRepeat(weight_repeat)) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseWgtRepeat] get the weight_repeat for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }

  if (weight_repeat == INT64_MAX) {
    return SUCCESS;
  }

  FE_LOGI("The weight repeat of node[%s] is %ld", op_desc_->GetName().c_str(), weight_repeat);
  if (!ge::AttrUtils::SetInt(op_desc_, ATTR_NAME_WEIGHT_REPEAT, weight_repeat)) {
    FE_LOGE("Fail to set attr weight_repeat for node[%s].", op_desc_->GetName().c_str());
    return FAILED;
  }
  return SUCCESS;
}
Status TbeJsonFileParse::ParseOpParaSize() {
  if (json_parser_vec_.empty()) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseOpParaSize] json parser is empty for node[%s].",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  int64_t op_para_size;
  if (json_parser_vec_[0].ParseOpParaSize(op_para_size) == FAILED) {
    REPORT_FE_ERROR("[SubGraphOpt][ParseJson][ParseOpParaSize] get the opParaSize for node[%s] failed.",
                    op_desc_->GetName().c_str());
    return FAILED;
  }
  (void)ge::AttrUtils::SetInt(op_desc_, OP_PARA_SIZE, (int64_t)op_para_size);
  FE_LOGD("ParseOpParaSize: %ld %s", op_para_size, op_desc_->GetName().c_str());
  return SUCCESS;
}


/*
*  @ingroup fe
*  @brief  joint the path of bin file, if success, renew the op_desc.name
*  @param   [in] handle
*  @param   [out] op_desc_->name
*  @return  SUCCESS or FAILED
*/
Status TbeJsonFileParse::PackageTvmBinFile() {
  vector<std::string> bin_file_path = tvm_file_path_vec_;
  for (size_t i = 0; i < json_parser_vec_.size(); i++) {
    vector<char> buffer;
    if (i < bin_file_path.size()) {
      op_desc_->SetExtAttr(ge::ATTR_NAME_OP_FILE_PATH, bin_file_path[i]);
      if (json_parser_vec_[i].PackageTvmBinFile(bin_file_path[i], buffer) != SUCCESS) {
        REPORT_FE_ERROR("[SubGraphOpt][ParseJson][PkgTvmBinFile] Read bin file failed, file path is %s.",
                        bin_file_path[i].c_str());
        return PARAM_INVALID;
      }
    }

  std::string kernel_name;
  ge::AttrUtils::GetStr(op_desc_, json_parser_vec_[i].GetAttrPrefix() + op_desc_->GetName() + "_kernelname",
                        kernel_name);
  ge::OpKernelBinPtr tbe_kernel_ptr = nullptr;
  FE_MAKE_SHARED(tbe_kernel_ptr = std::make_shared<ge::OpKernelBin>(kernel_name, std::move(buffer)), return FAILED);
  FE_CHECK(tbe_kernel_ptr == nullptr,
           REPORT_FE_ERROR("[SubGraphOpt][ParseJson][PkgTvmBinFile] tbeKernelPtr is nullptr."), return FAILED);
    op_desc_->SetExtAttr(json_parser_vec_[i].GetAttrPrefix() + ge::OP_EXTATTR_NAME_TBE_KERNEL, tbe_kernel_ptr);
    (void)ge::AttrUtils::SetStr(op_desc_,
        json_parser_vec_[i].GetAttrPrefix() + ge::ATTR_NAME_TBE_KERNEL_NAME, tbe_kernel_ptr->GetName());
    FE_LOGD("node[%s]'s tbe kernel name is %s.", op_desc_->GetName().c_str(), tbe_kernel_ptr->GetName().c_str());
    ge::Buffer tbe_kernel_buffer(tbe_kernel_ptr->GetBinDataSize());
    tbe_kernel_buffer = ge::Buffer::CopyFrom(tbe_kernel_ptr->GetBinData(), tbe_kernel_ptr->GetBinDataSize());
    (void)ge::AttrUtils::SetBytes(op_desc_,
        json_parser_vec_[i].GetAttrPrefix() + ge::ATTR_NAME_TBE_KERNEL_BUFFER, tbe_kernel_buffer);
    size_t tbe_kernel_size;
    tbe_kernel_size = tbe_kernel_ptr->GetBinDataSize();
    (void)ge::AttrUtils::SetInt(op_desc_, json_parser_vec_[i].GetAttrPrefix() + ATTR_NAME_TBE_KERNEL_SIZE, tbe_kernel_size);
    FE_LOGD("node[%s]'s tbe kernel buffer size is %lu.", op_desc_->GetName().c_str(), tbe_kernel_buffer.GetSize());
  }
  return SUCCESS;
}

/*
*  @ingroup fe
*  @brief   package the json info together
*  @param   [in]  info
*  @param   [out] tvm_file_path_
*  @return  SUCCESS or FAILED
*/
Status TbeJsonFileParse::PackageTvmJsonInfo(const std::string &json_file_path, const std::string &bin_file_path) {
  FE_LOGD("Start to parse op_json_file %s", json_file_path.c_str());
  vector<std::string> json_file_path_vec;
  vector<std::string> bin_file_path_vec;
  if (RealPath(json_file_path).empty()) {
    std::string mix_aic_json = GetSuffixJsonFile(json_file_path, "_mix_aic");
    std::string mix_aiv_json = GetSuffixJsonFile(json_file_path, "_mix_aiv");
    if (!RealPath(mix_aic_json).empty() && !RealPath(mix_aiv_json).empty()) {
      json_file_path_vec.push_back(mix_aic_json);
      json_file_path_vec.push_back(mix_aiv_json);
    } else {
      REPORT_FE_ERROR("[SubGraphOpt][ParseJson][PkgTvmJsInfo] ReadJsonFile failed.");
      return FAILED;
    }
  } else {
    json_file_path_vec.push_back(json_file_path);
  }
  bin_file_path_vec = json_file_path_vec;
  for (size_t i = 0; i < json_file_path_vec.size(); i++) {
    TbeJsonFileParseImpl json_parser;
    json_parser_vec_.push_back(json_parser);
    tvm_file_path_vec_.emplace_back("");
  }
  for (size_t i = 0; i < json_parser_vec_.size(); i++) {
    if (json_parser_vec_[i].Initialize(json_file_path_vec[i]) != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseJson][PkgTvmJsInfo] ReadJsonFile failed.");
      return FAILED;
    }

    tvm_file_path_vec_[i] = ".";
    size_t find_pos = bin_file_path_vec[i].find_last_of("\\/");
    if (find_pos != std::string::npos) {
      tvm_file_path_vec_[i] = bin_file_path_vec[i].substr(0, find_pos);
    }
  }

  for (auto &parseFunc : parse_func_map_) {
    if ((this->*(parseFunc.second))() != SUCCESS) {
      REPORT_FE_ERROR("[SubGraphOpt][ParseJson][PkgTvmJsInfo] Parse %s failed, the json file:%s.",
                      parseFunc.first.c_str(), json_file_path.c_str());
      return FAILED;
    }
  }
  return SUCCESS;
}
}  // namespace fe
