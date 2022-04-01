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
#include "prefetch_dynamic_task_builder.h"

namespace ffts {
PrefetchDynamicTaskBuilder::PrefetchDynamicTaskBuilder()
    : DataTaskBuilder(CACHE_OPERATION::PREFETCH) {}

PrefetchDynamicTaskBuilder::~PrefetchDynamicTaskBuilder() {}

Status PrefetchDynamicTaskBuilder::FillDynamicDataCtx(const size_t &in_anchor_index, const ge::NodePtr &node,
                                                      domi::FftsPlusTaskDef *ffts_plus_task_def,
                                                      const rtFftsPlusContextType_t &context_type,
                                                      const vector<uint32_t> &context_id_list) {
  FFTS_LOGD("node[%s] start to fill unknow shape prefetch data context.", node->GetName().c_str());
  std::vector<std::vector<int64_t>> data_prefetch_ctx_id_list;
  (void)ge::AttrUtils::GetListListInt(node->GetOpDesc(), "_data_prefetch_ctx_id_list", data_prefetch_ctx_id_list);
  for (size_t i = 0; i < context_id_list.size(); i++) {
    domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
    domi::FftsPlusDataCtxDef *data_ctx_def = ffts_plus_ctx_def->mutable_data_ctx();
    data_ctx_def->set_cnt_init(1);
    data_ctx_def->set_cnt(1);
    UpdateSrcSlotAndPfBm(ffts_plus_task_def, context_id_list[i]);

    if (data_prefetch_ctx_id_list.size() != context_id_list.size()) {
      vector<int64_t> data_prefetch_ctx_id = {ffts_plus_task_def->ffts_plus_ctx_size() - 1};
      data_prefetch_ctx_id_list.push_back(data_prefetch_ctx_id);
    } else {
      vector<int64_t> data_prefetch_ctx_id = data_prefetch_ctx_id_list[i];
      data_prefetch_ctx_id.push_back(ffts_plus_task_def->ffts_plus_ctx_size() - 1);
      data_prefetch_ctx_id_list[i] = data_prefetch_ctx_id;
    }
  }
  (void)ge::AttrUtils::SetListListInt(node->GetOpDesc(), "_data_prefetch_ctx_id_list", data_prefetch_ctx_id_list);
  return SUCCESS;
}
}
