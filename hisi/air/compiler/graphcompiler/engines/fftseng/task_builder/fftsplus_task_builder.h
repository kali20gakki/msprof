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

#ifndef FFTS_ENGINE_TASK_BUILDER_FFTSPLUS_TASK_BUILDER_H_
#define FFTS_ENGINE_TASK_BUILDER_FFTSPLUS_TASK_BUILDER_H_
#include <map>
#include <memory>
#include <vector>
#include "graph/compute_graph.h"
#include "inc/graph/utils/graph_utils.h"
#include "proto/task.pb.h"
#include "common/opskernel/ops_kernel_info_types.h"
#include "runtime/rt.h"
#include "common/sgt_slice_type.h"
#include "inc/ffts_log.h"

namespace ffts {

using FftsPlusComCtx_t = struct tagFftsPlusComCtx {
    uint16_t contextType;
    uint8_t successorNum;
    uint8_t pred_cnt;
    vector<uint32_t> succ_list;
};

using FftsPlusCtxDefPtr = std::shared_ptr<domi::FftsPlusCtxDef>;

class FFTSPlusTaskBuilder {
 public:
  FFTSPlusTaskBuilder();
  virtual ~FFTSPlusTaskBuilder();

  Status GenFftsPlusTaskCommonInfo(const ge::NodePtr &node, vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const;

  Status GenFftsPlusDependencyInfo(const ge::NodePtr &node, vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const;

  Status FillProducersInfo(const ge::NodePtr &node, FftsPlusComCtx_t &ffts_plus_context) const;
  void FillManualCustomersInfo(const ge::NodePtr &node, FftsPlusComCtx_t &sub_ffts_plus_context_elem) const;
  Status FillCustomersInfo(const ge::NodePtr &node, FftsPlusComCtx_t &sub_ffts_plus_context_elem,
                           vector<FftsPlusComCtx_t> &sub_ffts_plus_context) const;

  /*
   * @ingroup ffts
   * @brief   Generate tasks
   * @param   [in] node Node of compute graph
   * @param   [in] context Context for generate tasks
   * @param   [out] task_defs Save the generated tasks.
   * @return  SUCCESS or FAILED
   */
  Status GenerateTaskDef(const ge::NodePtr &node,
                         domi::FftsPlusTaskDef *ffts_plus_task_def);

  virtual Status GenContextDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) {
    return SUCCESS;
  };

  /* Create a label context and move the last succ id to this context. */
  template <typename T>
  static Status GenerateNewLabelCtx(domi::FftsPlusTaskDef *ffts_plus_task_def,
                                    uint32_t last_succ_id,
                                    T *pred_ctx,
                                    domi::FftsPlusLabelCtxDef **new_label) {
    uint32_t new_ctx_id = ffts_plus_task_def->ffts_plus_ctx_size();
    FFTS_LOGD("Generate a new label context. last_succ_id %u, new context id %u.", last_succ_id, new_ctx_id);
    domi::FftsPlusCtxDef* new_ctx = ffts_plus_task_def->add_ffts_plus_ctx();
    FFTS_CHECK_NOTNULL(new_ctx);
    new_ctx->set_context_type(RT_CTX_TYPE_LABEL);
    *new_label = new_ctx->mutable_label_ctx();
    FFTS_CHECK_NOTNULL(*new_label);
    /* We just create a new label. The caller of this function will put the
     * pending context id into the second position of this new label. */
    (*new_label)->add_successor_list(last_succ_id);
    (*new_label)->set_successor_num(1);
    (*new_label)->set_pred_cnt(1);
    (*new_label)->set_pred_cnt_init(1);

    pred_ctx->set_successor_list(RT_CTX_SUCCESSOR_NUM - 1, new_ctx_id);
    return SUCCESS;
  }

  static Status GetFirstAvailableLabel(domi::FftsPlusTaskDef *ffts_plus_task_def,
                                       domi::FftsPlusLabelCtxDef *pred_label_ctx,
                                       domi::FftsPlusLabelCtxDef **avl_label_context,
                                       uint32_t &recursion_count);

  /* If the ctx have 25 or less successor, we just add the successsor's id into it's
   * successor list.
   * And if the ctx have exactly 26 successors, we need to :
   * If the 26th context is label context:
   *   1. Get the next label context and check whether the next
   *      one is also full. If full, keep searching util we get a
   *      label context with less than 26 successor.
   *   2. put the succ_id into the final label context.
   *
   *
   * Else if the 26th is a normal context:
   *   1. Generate a new label context into whole sqe.
   *   2. Move the 26th successor id into the first
   *      position of the new label context.
   *   3. Put the succ_id into the second position of the new
   *      label context.
   *   4. Put the context id of the new label context(which
   *      is the size of all context - 1) into the 26th position of
   *      the normal context. */
  template <typename T>
  static Status AddOneId(domi::FftsPlusTaskDef *ffts_plus_task_def, uint32_t succ_id, T *ctx) {
    uint32_t succ_num = ctx->successor_num();
    if (succ_num == RT_CTX_SUCCESSOR_NUM) {
      uint32_t last_succ_id = ctx->successor_list(RT_CTX_SUCCESSOR_NUM - 1);
      uint32_t ctx_size = ffts_plus_task_def->ffts_plus_ctx_size();
      if (last_succ_id >= ctx_size) {
        REPORT_FFTS_ERROR("last_succ_id %u, ctx_size:%u", last_succ_id, ctx_size);
        return FAILED;
      }
      domi::FftsPlusCtxDef* last_succ_ctx =
          ffts_plus_task_def->mutable_ffts_plus_ctx(static_cast<int>(last_succ_id));
      FFTS_CHECK_NOTNULL(last_succ_ctx);
      domi::FftsPlusLabelCtxDef* avl_label_ctx = nullptr;
      if (last_succ_ctx->context_type() == RT_CTX_TYPE_LABEL) {
        FFTS_LOGD("last context is label, keep seaching its succsorrs.");
        domi::FftsPlusLabelCtxDef* pre_label = last_succ_ctx->mutable_label_ctx();
        uint32_t recursion_count = 0;
        if (GetFirstAvailableLabel(ffts_plus_task_def, pre_label, &avl_label_ctx, recursion_count) != SUCCESS ||
            avl_label_ctx == nullptr) {
          REPORT_FFTS_ERROR("Cannot find any available label context for succ_id %u.", succ_id);
          return FAILED;
        }
      } else {
        /* Just generate a label context and move the last successor into the
         * new generated label and return it. */
        if (GenerateNewLabelCtx(ffts_plus_task_def, last_succ_id, ctx, &avl_label_ctx) != SUCCESS) {
          return FAILED;
        }
      }
      FFTS_CHECK_NOTNULL(avl_label_ctx);
      FFTS_LOGD("Add one successor %u for label.", succ_id);
      avl_label_ctx->add_successor_list(succ_id);
      succ_num = static_cast<uint32_t>(avl_label_ctx->successor_list_size());
      FFTS_LOGD("succ_num %u for label.", succ_num);
      avl_label_ctx->set_successor_num(succ_num);
    } else {
      ++succ_num;
      FFTS_LOGD("Add one successor %u. successor num %u.", succ_id, succ_num);
      ctx->set_successor_num(succ_num);
      ctx->add_successor_list(succ_id);
    }
    return SUCCESS;
  }

  Status UpdateSuccList(uint32_t succ_id, uint32_t curr_id, domi::FftsPlusTaskDef *ffts_plus_task_def) const;

  template <typename T>
  static Status add_at_end_to_write_back_succ_list(const uint32_t &at_end_ctx_id, T *ctx,
                                                   domi::FftsPlusTaskDef *ffts_plus_task_def, bool &already_add) {
    uint32_t succ_num = ctx->successor_num();
    for (size_t i = 0; i < static_cast<size_t>(succ_num); i++) {
      uint32_t cur_succ_id = ctx->successor_list(i);
      domi::FftsPlusCtxDef* cur_succ_ctx = ffts_plus_task_def->mutable_ffts_plus_ctx(static_cast<int>(cur_succ_id));
      FFTS_CHECK_NOTNULL(cur_succ_ctx);
      auto type = cur_succ_ctx->context_type();
      if (type == RT_HW_CTX_TYPE_WRITEBACK_DATA) {
        domi::FftsPlusDataCtxDef* write_back_ctx = cur_succ_ctx->mutable_data_ctx();
        write_back_ctx->add_successor_list(at_end_ctx_id);
        write_back_ctx->set_successor_num(1);
        if (already_add) {
          domi::FftsPlusCtxDef* common_ctx = ffts_plus_task_def->mutable_ffts_plus_ctx(static_cast<int>(at_end_ctx_id));
          domi::FftsPlusAtEndCtxDef* at_end_ctx = common_ctx->mutable_at_end_ctx();
          at_end_ctx->set_pred_cnt(at_end_ctx->pred_cnt() + 1);
          at_end_ctx->set_pred_cnt_init(at_end_ctx->pred_cnt_init() + 1);
        }
        already_add = true;
      }
    }
    return SUCCESS;
  }

 protected:
  void FillContextData(const domi::FftsPlusMixAicAivCtxDef *aicore_ctx_def,
                       domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def) const;

  void FillContextData(const domi::FftsPlusAicAivCtxDef *aicore_ctx_def,
                       domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def) const;

  Status FillContextData(const domi::FftsPlusAicpuCtxDef *aicpu_ctx_def_ptr,
                         domi::FftsPlusAicpuCtxDef *aicpu_ctx_def) const;
 private:
  FFTSPlusTaskBuilder(const FFTSPlusTaskBuilder &builder) = delete;

  FFTSPlusTaskBuilder &operator=(const FFTSPlusTaskBuilder &builder) = delete;
};

}  // namespace ffts
#endif  // FUSION_ENGINE_UTILS_TASK_BUILDER_TASK_BUILDER_H_
