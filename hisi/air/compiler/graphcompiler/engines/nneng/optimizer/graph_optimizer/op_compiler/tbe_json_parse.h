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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_JSON_PARSE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_JSON_PARSE_H_

#include <climits>
#include <string>
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "graph_optimizer/ffts/tbe_json_parse_impl.h"

using JsonHandle = void*;

namespace fe {

class TbeJsonFileParse {
 public:
  explicit TbeJsonFileParse(ge::Node& node) : node_(node), op_desc_(node.GetOpDesc()) {};

  ~TbeJsonFileParse() {};

  /*
  *  @ingroup fe
  *  @brief   package the json info together
  *  @param   [in]  info
  *  @param   [out] tvm_file_path_
  *  @return  SUCCESS or FAILED
  */
  Status PackageTvmJsonInfo(const std::string &json_file_path, const std::string &bin_file_path);

 private:
  ge::Node& node_;
  vector<std::string> tvm_file_path_vec_;
  ge::OpDescPtr op_desc_{nullptr};
  vector<TbeJsonFileParseImpl> json_parser_vec_;
  using ParseFunc = Status(TbeJsonFileParse::*)();
  std::vector<std::pair<std::string, ParseFunc>> parse_func_map_ = {
    {"core_type", &TbeJsonFileParse::ParseTvmCoreType},
    {"magic", &TbeJsonFileParse::ParseTvmMagic},
    {"blockDim", &TbeJsonFileParse::ParseTvmBlockDim},
    {"taskRation", &TbeJsonFileParse::ParseTvmTaskRatio},
    {"modeInArgsFirstField", &TbeJsonFileParse::ParseTvmModeInArgsFirstField},
    {"batchBindOnly", &TbeJsonFileParse::ParseBatchBindOnly},
    {"workspace", &TbeJsonFileParse::ParseTvmWorkSpace},
    {"parameters", &TbeJsonFileParse::ParseTvmParameters},
    {"metaData", &TbeJsonFileParse::ParseTvmMetaData},
    {"kernelName", &TbeJsonFileParse::ParseTvmKernelName},
    {"compress_parameters", &TbeJsonFileParse::ParseConvCompressParameters},
    {"weight_repeat", &TbeJsonFileParse::ParseWeightRepeat},
    {"opParaSize", &TbeJsonFileParse::ParseOpParaSize},
    {"BinFile", &TbeJsonFileParse::PackageTvmBinFile},
    {"kernelList", &TbeJsonFileParse::ParseTvmKernelList},
    {"globleworkspace", &TbeJsonFileParse::ParseGlobleWorkspaceStatus},
    {kKBHit, &TbeJsonFileParse::ParseOpKBHitrate}
  };

  TbeJsonFileParse& operator=(const TbeJsonFileParse& op) { if (&op == this) { return *this; } return *this; }

  /*
  *  @ingroup fe
  *  @brief  joint the path of bin file, if success, renew the op_desc.name
  *  @param   [in] handle
  *  @param   [out] op_desc_->name
  *  @return  SUCCESS or FAILED
  */
  Status PackageTvmBinFile();

  /*
  *  @ingroup fe
  *  @brief parse the magic info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_ set magic according to magic info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmMagic();

  /*
  *  @ingroup fe
  *  @brief  parse the block_dim info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_ set block_dim according to the block_dim info in
  *  handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmBlockDim();

  /*
   *  @ingroup fe
   *  @brief  parse the batch_bind_only info in handle
   *  @param   [in] handle
   *  @param   [out] op_desc_ set batch_bind_only according to
   *  the batch_bind_only info in handle
   *  @return SUCCESS or FAILED
   */
  Status ParseBatchBindOnly();

  /*
  *  @ingroup fe
  *  @brief  parse the workspace info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, tvm_workspace_sizes_
  *  set workspace according to block_dim info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmWorkSpace();

  /*
  *  @ingroup fe
  *  @brief  parse the parameters info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set output according to output info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmParameters();

  /*
  *  @ingroup fe
  *  @brief  parse the meta_data info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set meta_data according to meta_data info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmMetaData();

  /*
  *  @ingroup fe
  *  @brief  parse the kernel_name info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, add kernel_name to op_desc_.name according to
  *  kernel_name info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseTvmKernelName();

  /*
  *  @ingroup fe
  *  @brief  parse the compress_parameters info in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set compress_parameters to op_desc_
  *  according to compress_parameters info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseConvCompressParameters();

  /*
  *  @ingroup fe
  *  @brief  parse the op_para_size in handle
  *  @param   [in] handle
  *  @param   [out] op_desc_, set op_para_size to op_desc_
  *  according to compress_parameters info in handle
  *  @return SUCCESS or FAILED
  */
  Status ParseOpParaSize();

  /*
   *  @ingroup fe
   *  @brief  parse the weight_repeat info in handle
   *  @param   [in] handle
   *  @param   [out] op_desc_, set _weight_repeat to op_desc_
   *  according to weight_repeat info in handle
   *  @return SUCCESS or FAILED
   */
  Status ParseWeightRepeat();

  Status ParseTvmCoreType();

  Status ParseTvmTaskRatio();

  Status ParseTvmModeInArgsFirstField();

  Status ParseTvmKernelList();

  Status ParseGlobleWorkspaceStatus();

  Status ParseOpKBHitrate();

  void GetWorkspaceAtomicFlagAndOutputIndexFlag(const std::vector<int64_t> &parameters_index,
                                                const size_t &workspace_num, const size_t &input_num,
                                                const size_t &output_num, std::vector<int64_t> &output_index,
                                                std::vector<int64_t> &workspace_index,
                                                bool &workspace_atomic_flag, bool &output_index_flag);

  Status SetAtomicInfo(std::vector<int64_t> &parameters_index);

};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_JSON_PARSE_H_
