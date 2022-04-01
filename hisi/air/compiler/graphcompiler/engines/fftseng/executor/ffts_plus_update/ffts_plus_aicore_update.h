/**
 * Copyright 2022-2023 Huawei Technologies Co., Ltd
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

#ifndef FFTS_ENGIINE_EXECUTOR_FFTS_PLUS_AICORE_UPDATE_H_
#define FFTS_ENGIINE_EXECUTOR_FFTS_PLUS_AICORE_UPDATE_H_
#include "register/ffts_plus_task_update.h"
#include "register/ffts_plus_update_manager.h"
#include "graph/utils/graph_utils.h"
#include "graph/utils/tensor_utils.h"
#include "common/sgt_slice_type.h"
#include "inc/ffts_log.h"
#include "inc/ffts_error_codes.h"
#include "inc/ffts_type.h"
#include "inc/memory_slice.h"
#include "runtime/rt_ffts_plus_define.h"
#include "runtime/rt_ffts_plus.h"

namespace ffts {
const int kMaxPrefetchNum = 4;
const int kMaxCmoType = 3;
const int kEachParamNum = 2;
const int64_t kMaxIndexNum = 64;
class FFTSPlusAICoreUpdate : public ge::FFTSPlusTaskUpdate {
 public:
  FFTSPlusAICoreUpdate();
  ~FFTSPlusAICoreUpdate() override;
  // Interface to calculate auto thread param
  Status GetAutoThreadParam(const ge::NodePtr &node, const vector<optiling::utils::OpRunInfo> &op_run_info,
                            ge::AutoThreadParam &auto_thread_param) override;
  // Interface to update AICore context
  Status UpdateSubTaskAndCache(const ge::NodePtr &node, const ge::AutoThreadSubTaskFlush &flush_data,
                               rtFftsPlusTaskInfo_t &task_info) override;
 private:
  Status CalcAutoThreadInput(const ge::NodePtr &node, vector<vector<vector<ffts::DimRange>>> &tensor_slice,
                             ge::AutoThreadParam &argsPara);
  Status CalcAutoThreadOutput(const ge::NodePtr &node, vector<vector<vector<ffts::DimRange>>> &tensor_slice,
                              ge::AutoThreadParam &argsPara);
  Status CalcAutoThreadWorkspace(const vector<optiling::utils::OpRunInfo> &op_run_info, ge::AutoThreadParam &args_para);

  // update context function
  Status UpdateAicAivCtx(rtFftsPlusComCtx_t *com_ctx, const ge::AutoThreadSubTaskFlush &flush_data) const;
  Status UpdateMixL2AicAivCtx(const rtFftsPlusTaskInfo_t &task_info,
                              const ge::AutoThreadSubTaskFlush &flush_data, const ge::NodePtr node) const;
  Status UpdateContextByType(const ge::NodePtr node, const ge::AutoThreadSubTaskFlush &flush_data,
                             const rtFftsPlusTaskInfo_t &task_info, const vector<int32_t> &ctx_id_vec);

  Status FillInvAndWriCtxParam(rtFftsPlusDataCtx_t* ctx, const ge::AutoThreadSubTaskFlush &flush_data,
                               size_t index) const;
	Status FillPrefetchCtxParam(rtFftsPlusDataCtx_t* ctx, const ge::AutoThreadSubTaskFlush &flush_data,
                              size_t index) const;
  Status UpdateCmoCtxProc(const rtFftsPlusTaskInfo_t &task_info, const ge::NodePtr &node,
                          const ge::AutoThreadSubTaskFlush &flush_data, int type, int data_num);
  Status UpdateCmoProc(const rtFftsPlusTaskInfo_t &task_info, const ge::NodePtr &node,
                       const ge::AutoThreadSubTaskFlush &flush_data);
  Status GenerateInputCmoParam(const ge::NodePtr &node, int real_index, uint32_t index);
  Status GenerateOutputCmoParam(const ge::NodePtr &node, int real_index, uint32_t index);
  uint32_t input_num_;
  uint32_t input_output_num_;
  uint64_t task_param_offset_;
  ffts::ThreadSliceMapPtr slice_info_ptr_;
  DataContextParam *data_params_;
};

inline Status GetDataTypeSize(const ge::DataType &data_type, uint32_t &data_type_size) {
  int res = ge::GetSizeByDataType(data_type);
  if (res < 0) {
    return FAILED;
  }
  data_type_size = static_cast<uint32_t>(res);
  return SUCCESS;
}

inline void SetLow32FromSrc(uint32_t &dst, const uint64_t &src)
{
  dst = static_cast<uint32_t>(src & 0xFFFFFFFF);
  return;
}
inline void SetHigh32FromSrc(uint32_t &dst, const uint64_t &src)
{
  dst = static_cast<uint32_t>((src >> 32) & 0xFFFFFFFF);
  return;
}
inline void SetHigh16FromSrc(uint16_t &dst, const uint64_t &src)
{
  dst = static_cast<uint16_t>((src >> 32) & 0xFFFF);
  return;
}

Status GenerateDataCtxParam(ge::GeTensorDescPtr tensor, const std::vector<DimRange> &slice,
                            DataContextParam* data_ctx, uint32_t index);
}

#endif
