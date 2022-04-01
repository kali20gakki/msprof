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
#include "ffts_plus_engine_update.h"
#include "inc/ffts_log.h"
#include "inc/ffts_utils.h"
#include "inc/ffts_error_codes.h"
#include "common/sgt_slice_type.h"
#include "runtime/rt_ffts_plus_define.h"
#include "inc/graph/debug/ge_attr_define.h"
namespace ffts {
FFTSPlusEngineUpdate::FFTSPlusEngineUpdate() {}
FFTSPlusEngineUpdate::~FFTSPlusEngineUpdate() {}

void UpdateStartCtx(rtFftsPlusComCtx_t *comCtx, ffts::ThreadSliceMapPtr slice_info_ptr)
{
  rtFftsPlusAtStartCtx_t* ctx = reinterpret_cast<rtFftsPlusAtStartCtx_t*>(comCtx);
  ctx->threadDim = slice_info_ptr->slice_instance_num;
  ctx->threadWindowSize = slice_info_ptr->parallel_window_size;
  FFTS_LOGD("Update startCtx thread_dim[%u] windowSize[%u].", ctx->threadDim, ctx->threadWindowSize);
  return;
}

void UpdateLabelCtx(rtFftsPlusComCtx_t *comCtx, const rtFftsPlusTaskInfo_t &task_info,
                    ffts::ThreadSliceMapPtr slice_info_ptr, int32_t idx)
{
  if (comCtx == nullptr) {
    return;
  }
  rtFftsPlusLabelCtx_t* ctx = reinterpret_cast<rtFftsPlusLabelCtx_t*>(comCtx);
  if (idx == 0) {
    // compile create num > runtime thread dim
    FFTS_LOGD("Thread_dim: %u, window_size: %u.", slice_info_ptr->slice_instance_num,
              slice_info_ptr->parallel_window_size);
    if (ctx->successorNum == slice_info_ptr->parallel_window_size) {
      return;
    }
    FFTS_LOGD("Set In label successor num (%d).", slice_info_ptr->parallel_window_size);
    ctx->successorNum = slice_info_ptr->parallel_window_size;
    return;
  }
  ctx->predCnt = slice_info_ptr->slice_instance_num;
  ctx->predCntInit = slice_info_ptr->slice_instance_num;
  FFTS_LOGD("Update Out label predcnt %u.", ctx->predCnt);
  return;
}

bool FFTSPlusEngineUpdate::UpdateCommonCtx(ge::ComputeGraphPtr &sgt_graph, rtFftsPlusTaskInfo_t &task_info)
{
  std::string op_name;
  (void)ge::AttrUtils::GetStr(sgt_graph, kFftsFirstOpName, op_name);
  const ge::NodePtr node = sgt_graph->FindNode(op_name);
  FFTS_CHECK_NOTNULL(node);
  ge::OpDescPtr op_desc = node->GetOpDesc();
  FFTS_LOGD("Update Common Ctx from node(%s).", op_desc->GetName().c_str());
  ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = op_desc->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
  if (slice_info_ptr == nullptr) {
    FFTS_LOGE("[Executor][FFTS_plus_update][UpdateCommonCtx]Slice info is null.");
    return false;
  }
  void *ctx_buf = const_cast<void*>(task_info.descBuf);
  rtFftsPlusComCtx_t *context_head = reinterpret_cast<rtFftsPlusComCtx_t*>(ctx_buf);
  rtFftsPlusComCtx_t *context = nullptr;
  static std::vector<int32_t> ext_id_vec;
  (void)ge::AttrUtils::GetListInt(op_desc, "_all_ctx_id_list", ext_id_vec);
  if (ext_id_vec.size() > task_info.fftsPlusSqe->totalContextNum) {
    FFTS_LOGE("[Executor][FFTS_plus_update][UpdateCommonCtx]Context size(%zu) over err.", ext_id_vec.size());
    return false;
  }
  for (auto idx = ext_id_vec.begin(); idx != ext_id_vec.end(); ++idx) {
    context = context_head + (*idx);
    if (context == nullptr) {
      FFTS_LOGE("[Executor][FFTS_plus_update][UpdateCommonCtx]Context(%d) is null.", *idx);
      return false;
    }
    FFTS_LOGD("Context Id[%d] , type[0x%x].", *idx, context->contextType);
    if (context->contextType == RT_CTX_TYPE_AT_START) {
      UpdateStartCtx(context, slice_info_ptr);
    } else if (context->contextType == RT_CTX_TYPE_LABEL) {
      UpdateLabelCtx(context, task_info, slice_info_ptr, *idx);
    }
  }
  return true;
}
}