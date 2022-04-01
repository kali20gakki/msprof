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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_JSON_PARSE_IMPL_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_JSON_PARSE_IMPL_H_

#include <climits>
#include <string>
#include <vector>
#include "common/fe_inner_error_codes.h"
#include "graph/compute_graph.h"
#include <nlohmann/json.hpp>
using JsonHandle = void*;

namespace fe {
static const int MAX_FILE_SIZE_LIMIT = INT_MAX;

const std::vector<std::string> kBinaryMagicTypesVec = {
        "RT_DEV_BINARY_MAGIC_ELF_AICPU",
        "RT_DEV_BINARY_MAGIC_ELF",
        "RT_DEV_BINARY_MAGIC_ELF_AIVEC",
        "RT_DEV_BINARY_MAGIC_ELF_AICUBE",
        "FFTS_BINARY_MAGIC_ELF_MIX_AIC",
        "FFTS_BINARY_MAGIC_ELF_MIX_AIV"
};

class TbeJsonFileParseImpl {
 public:
  TbeJsonFileParseImpl() {};

  ~TbeJsonFileParseImpl() {};

  /*
  *  @ingroup fe
  *  @brief   package the json info together
  *  @param   [in]  info
  *  @param   [out] tvm_file_path_
  *  @return  SUCCESS or FAILED
  */
  Status Initialize(const std::string &json_file_path);

  TbeJsonFileParseImpl & operator=(const TbeJsonFileParseImpl & op) { if (&op == this) { return *this; } return *this; }

  /*
  *  @ingroup fe
  *  @brief   reading binary files
  *  @param   [in]  file_name(or path)
  *  @param   [out] buffer
  *  @return  SUCCESS or FAILED
  */
  Status ReadBytesFromBinaryFile(const std::string& file_name, std::vector<char>& buffer);

  /*
  *  @ingroup fe
  *  @brief  joint the path of bin file, if success, renew the op_desc.name
  *  @param   [in] handle
  *  @param   [out] op_desc_->name
  *  @return  SUCCESS or FAILED
  */
  Status PackageTvmBinFile(std::string &bin_file_path, std::vector<char> &buffer);

  /*
  *  @ingroup fe
  *  @brief parse the magic info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_ set magic according to magic info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmMagic(std::string &magic);

  /*
  *  @ingroup fe
  *  @brief  parse the block_dim info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_ set block_dim according to the block_dim info in
  *  handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmBlockDim(int32_t &block_dim);

  /*
   *  @ingroup fe
   *  @brief  parse the batch_bind_only info in handle
   *  @param   [in] handle
   *  @param   [out] op_desc_ set batch_bind_only according to
   *  the batch_bind_only info in handle
   *  @return SUCCESS or FAILED
   */
  Status ParseBatchBindOnly(uint32_t &batch_bind_only);

  /*
  *  @ingroup fe
  *  @brief  parse the workspace info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, tvm_workspace_sizes_
  *  set workspace according to block_dim info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmWorkSpace(std::vector<int64_t> &tvm_workspace_sizes, std::vector<int64_t> &tvm_workspace_types);

  /*
  *  @ingroup fe
  *  @brief  parse the parameters info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set output according to output info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmParameters(std::vector<int64_t> &parameters_index);

  /*
  *  @ingroup fe
  *  @brief  parse the meta_data info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set meta_data according to meta_data info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmMetaData(std::string &meta_data);

  /*
  *  @ingroup fe
  *  @brief  parse the kernel_name info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, add kernel_name to op_desc_.name according to
  *  kernel_name info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmKernelName(std::string &kernel_name);

  /*
  *  @ingroup fe
  *  @brief  parse the compress_parameters info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set compress_parameters to op_desc_
  *  according to compress_parameters info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseConvCompressParameters(std::vector<int64_t> &compress_param_vec);

  /*
  *  @ingroup fe
  *  @brief  parse the op_para_size in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set op_para_size to op_desc_
  *  according to compress_parameters info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseOpParaSize(int64_t &op_para_size);

  /*
   *  @ingroup fe
   *  @brief  parse the weight_repeat info in handle
   *  @param   [in] handle
   *  @param   [out] op_desc_, set _weight_repeat to op_desc_
   *  according to weight_repeat info in handle
   *  @return SUCCESS or FAILED
   */
  Status ParseWeightRepeat(int64_t &weight_repeat);

  /*
   *  @ingroup fe
   *  @brief  get the str value from handle based on key
   *  @param   [in] handle, key
   *  @return j->at(key)
   */
  std::string GetStrValueFromJson(const std::string& key);

  Status ParseGlobleWorkspaceStatus(ge::ComputeGraphPtr &graph, ge::OpDescPtr &op_desc_ptr);

  Status ParseTvmCoreType(std::string &core_type);

  Status ParseTvmTaskRatio(uint32_t &task_ratio);

  Status ParseModeInArgsFirstField(uint32_t &mode);

  Status ParseTvmKernelList(std::string &kernel_list_first);

  Status ParseOpKBHitrate(int32_t &op_hitrate);

  std::string GetAttrPrefix();

 private:
  JsonHandle handle_;

  nlohmann::json j_;

  std::string attr_prefix_;
};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_JSON_PARSE_BASE_H_
