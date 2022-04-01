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

#ifndef FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_SGT_JSON_PARSE_H_
#define FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_SGT_JSON_PARSE_H_

#include <climits>
#include <string>
#include <vector>
#include "common/fe_inner_attr_define.h"
#include "common/fe_inner_error_codes.h"
#include "framework/omg/omg_inner_types.h"
#include "graph/op_desc.h"
#include "graph/utils/attr_utils.h"
#include "tbe_json_parse_impl.h"
#include "common/ffts_plus_type.h"

using JsonHandle = void*;

namespace fe {
/* There are only two separate json file for sgt slicing cases.
 * The first */
const uint32_t kTotalSgtJsonNum = 2;
const uint32_t kNonTailSgtJsonNum = 1;
const uint32_t kTailSgtJsonNum = 1;
const size_t kThreadJsonNumMax = 2;
const size_t kThreadNumMax = 2;

class TbeSgtJsonFileParse {
 public:
  explicit TbeSgtJsonFileParse(ge::Node& node) : node_(node), op_desc_(node.GetOpDesc()) {};

  ~TbeSgtJsonFileParse() {};

  /*
  *  @ingroup fe
  *  @brief   package the json info together
  *  @param   [in]  info
  *  @param   [out] tvm_file_path_
  *  @return  SUCCESS or FAILED
  */
  Status PackageTvmJsonInfo(const vector<std::string> &json_file_path_vec, vector<std::string> &bin_file_path_vec);
  void PackageTvmJsonInfoSub(size_t &kCoreTypeNum, vector<std::string> &bin_file_path_vec);
 private:
  ge::Node& node_;
  vector<vector<std::string>> tvm_file_path_vec_;
  ge::OpDescPtr op_desc_{nullptr};
  /* Here we need to merge all non-tail threads' workspace bytes. */
  ffts::ThreadSliceMapPtr slice_info_ptr_{nullptr};
  vector<vector<TbeJsonFileParseImpl>> json_parser_vec_;
  using parse_func = Status(TbeSgtJsonFileParse::*)();
  std::vector<std::pair<std::string, parse_func>> parse_func_map = {
      {"core_type", &TbeSgtJsonFileParse::ParseTvmCoreType},
      {"magic", &TbeSgtJsonFileParse::ParseTvmMagic},
      {"blockDim", &TbeSgtJsonFileParse::ParseTvmBlockDim},
      {"taskRatio", &TbeSgtJsonFileParse::ParseTvmTaskRatio},
      {"modeInArgsFirstField", &TbeSgtJsonFileParse::ParseTvmModeInArgsFirstField},
      {"batchBindOnly", &TbeSgtJsonFileParse::ParseBatchBindOnly},
      {"workspace", &TbeSgtJsonFileParse::ParseTvmWorkSpace},
      {"parameters", &TbeSgtJsonFileParse::ParseTvmParameters},
      {"metaData", &TbeSgtJsonFileParse::ParseTvmMetaData},
      {"kernelName", &TbeSgtJsonFileParse::ParseTvmKernelName},
      {"compress_parameters", &TbeSgtJsonFileParse::ParseConvCompressParameters},
      {"weight_repeat", &TbeSgtJsonFileParse::ParseWeightRepeat},
      {"opParaSize", &TbeSgtJsonFileParse::ParseOpParaSize},
      {"BinFile", &TbeSgtJsonFileParse::PackageTvmBinFile},
      {"globleworkspace", &TbeSgtJsonFileParse::ParseGlobleWorkspaceStatus},
      {kKBHit, &TbeSgtJsonFileParse::ParseOpKBHitrate}
  };

  TbeSgtJsonFileParse& operator=(const TbeSgtJsonFileParse& op) { if (&op == this) { return *this; } return *this; }

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

  Status ParseOpKBHitrate();

  Status ParseTvmModeInArgsFirstField();

  Status ParseGlobleWorkspaceStatus();

  void GetWorkspaceAtomicFlagAndOutputIndexFlag(const std::vector<int64_t> &parameters_index,
                                                const size_t &workspace_num, const size_t &input_num,
                                                const size_t &output_num, std::vector<int64_t> &output_index,
                                                int64_t &workspace_atomic_flag, bool &output_index_flag);

  Status SetAtomicInfo(std::vector<int64_t> &parameters_index,
                       std::vector<int64_t> &ub_atomic_params,
                       int64_t &workspace_atomic_flag,
                       std::vector<int64_t> &output_index);

};
}  // namespace fe

#endif  // FUSION_ENGINE_OPTIMIZER_GRAPH_OPTIMIZER_OP_COMPILER_TBE_JSON_PARSE_H_

