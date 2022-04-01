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
#include "out_dynamic_task_builder.h"

namespace ffts {
const size_t kDynamicWindowSize = 4;

OutDynamicTaskBuilder::OutDynamicTaskBuilder() {}

OutDynamicTaskBuilder::OutDynamicTaskBuilder(CACHE_OPERATION operation)
    : DataTaskBuilder(operation) {}

OutDynamicTaskBuilder::~OutDynamicTaskBuilder() {}

void OutDynamicTaskBuilder::RecordNewCtxIdList(vector<uint32_t> &data_ctx_id_list,
                                               vector<vector<int64_t>> &write_back_invalid_ctx_id_list,
                                               domi::FftsPlusTaskDef *ffts_plus_task_def,
                                               const rtFftsPlusContextType_t &context_type) const {
  for (size_t i = 0; i < kDynamicWindowSize; i++) {
    domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
    ffts_plus_ctx_def->set_context_type(context_type);
    domi::FftsPlusDataCtxDef *data_ctx_def = ffts_plus_ctx_def->mutable_data_ctx();
    data_ctx_def->set_cnt(1);
    data_ctx_def->set_cnt_init(1);
    vector<int64_t> pre_ctx_id = write_back_invalid_ctx_id_list[i];
    pre_ctx_id.push_back(ffts_plus_task_def->ffts_plus_ctx_size() - 1);
    data_ctx_id_list.push_back(ffts_plus_task_def->ffts_plus_ctx_size() - 1);
    write_back_invalid_ctx_id_list[i] = pre_ctx_id;
  }
}

Status OutDynamicTaskBuilder::FillDynamicDataCtx(const size_t &out_anchor_index, const ge::NodePtr &node,
                                                 domi::FftsPlusTaskDef *ffts_plus_task_def,
                                                 const rtFftsPlusContextType_t &context_type,
                                                 const vector<uint32_t> &context_id_list) {
  FFTS_LOGD("node[%s] start to fill unknow shape write_back and invalid data context.", node->GetName().c_str());
  if (operation_ == CACHE_OPERATION::WRITE_BACK) {
    vector<vector<int64_t>> write_back_ctx_id_list(kDynamicWindowSize, vector<int64_t>(0, 0));
    (void)ge::AttrUtils::GetListListInt(node->GetOpDesc(), "_write_back_ctx_id_list", write_back_ctx_id_list);

    vector<uint32_t> data_ctx_id_list;
    RecordNewCtxIdList(data_ctx_id_list, write_back_ctx_id_list, ffts_plus_task_def, context_type);

    if (context_id_list.size() != kDynamicWindowSize) {
      REPORT_FFTS_ERROR("[DataTaskBuilder][GenContextDef][FillDynamicInvalidAndWriteBackCtx] Node: %s context_id_list \
                        size is not equal to kDynamicWindowSize: %zu", node->GetName().c_str(), kDynamicWindowSize);
      return FAILED;
    }

    for (size_t i = 0; i < kDynamicWindowSize; i++) {
      UpdateSuccList(data_ctx_id_list[i], context_id_list[i], ffts_plus_task_def);
    }

    (void)ge::AttrUtils::SetListListInt(node->GetOpDesc(), "_write_back_ctx_id_list", write_back_ctx_id_list);
  } else {
    vector<vector<int64_t>> invalid_ctx_id_list(kDynamicWindowSize, vector<int64_t>(0, 0));
    (void)ge::AttrUtils::GetListListInt(node->GetOpDesc(), "_invalid_ctx_id_list", invalid_ctx_id_list);

    vector<uint32_t> data_ctx_id_list;
    RecordNewCtxIdList(data_ctx_id_list, invalid_ctx_id_list, ffts_plus_task_def, context_type);

    // generate succ_list
    vector<uint32_t> succ_list;
    uint32_t const_cnt;
    GetSuccessorContextId(out_anchor_index, node, succ_list, const_cnt);
    for (size_t i = 0; i < kDynamicWindowSize; i++) {
      for (size_t j = 0; j < succ_list.size(); j++) {
        UpdateSuccList(data_ctx_id_list[i], succ_list[j] + i, ffts_plus_task_def);
      }
    }

    (void)ge::AttrUtils::SetListListInt(node->GetOpDesc(), "_invalid_ctx_id_list", invalid_ctx_id_list);
  }
  return SUCCESS;
}
}