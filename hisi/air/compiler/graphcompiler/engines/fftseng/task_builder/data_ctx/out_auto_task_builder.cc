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
#include "out_auto_task_builder.h"

namespace ffts {
OutAutoTaskBuilder::OutAutoTaskBuilder() {}

OutAutoTaskBuilder::OutAutoTaskBuilder(CACHE_OPERATION operation)
    : DataTaskBuilder(operation) {}

OutAutoTaskBuilder::~OutAutoTaskBuilder() {}

Status OutAutoTaskBuilder::UptSuccListOfRelatedNode(const ge::NodePtr &node, const std::vector<uint32_t> &succ_list,
                                                    domi::FftsPlusTaskDef *ffts_plus_task_def,
                                                    const size_t &window_id) const {
  /* If original context size is 9 and after generate this out data context,
   * the total size is 10. So the context id of this out data context is 10 - 1 = 9; */
  auto data_ctx_id = ffts_plus_task_def->ffts_plus_ctx_size() - 1;
  FFTS_LOGD("Data context id is %d.", data_ctx_id);

  if (operation_ == CACHE_OPERATION::WRITE_BACK) {
    auto op_desc = node->GetOpDesc();
    vector<uint32_t> context_id_list;
    (void)ge::AttrUtils::GetListInt(op_desc, kAutoCtxIdList, context_id_list);
    if (context_id_list.size() > window_id) {
      UpdateSuccList(data_ctx_id, context_id_list[window_id], ffts_plus_task_def);
    }
  } else {
    for (auto target_id : succ_list) {
      UpdateSuccList(data_ctx_id, target_id + window_id, ffts_plus_task_def);
    }
  }
  return SUCCESS;
}

Status OutAutoTaskBuilder::FillAutoDataCtx(size_t out_anchor_index, const ge::NodePtr &node,
                                           const std::vector<DataContextParam> &params,
                                           domi::FftsPlusTaskDef *ffts_plus_task_def,
                                           domi::FftsPlusDataCtxDef *data_ctx_def, const size_t &window_id) {
  auto op_desc = node->GetOpDesc();
  vector<uint32_t> context_id_list;
  if (!ge::AttrUtils::GetListInt(op_desc, kAutoCtxIdList, context_id_list)) {
    REPORT_FFTS_ERROR("[GenTsk][DataTsk][FillAutoCtxt][node %s, type %s] Cannot get context id (list) for this node.",
                      node->GetName().c_str(), node->GetType().c_str());
    return FAILED;
  }

  data_ctx_def->set_aten(1);

  uint32_t cons_cnt = 0;
  std::vector<uint32_t> succ_list;
  if (operation_ == CACHE_OPERATION::WRITE_BACK) {
    /* In write back case, the write back data context can only be
     * executed when the only producer has already been executed. */
    data_ctx_def->set_cnt_init(1);
    data_ctx_def->set_cnt(1);
    FFTS_LOGD("Fill write back for node %s, consumer count is 1.", node->GetName().c_str());
  } else {
    /* In invalidate case, the invalidate data context can only be
     * executed when all the consumers have already been executed. */
    GetSuccessorContextId(out_anchor_index, node, succ_list, cons_cnt);
    data_ctx_def->set_cnt_init(cons_cnt);
    data_ctx_def->set_cnt(cons_cnt);
    data_ctx_def->set_orig_consumer_counter(cons_cnt);
    data_ctx_def->set_run_consumer_counter(cons_cnt);
    FFTS_LOGD("Fill invalidate for node %s, consumer count is %u.", node->GetName().c_str(),
              cons_cnt);
  }

  UptSuccListOfRelatedNode(node, succ_list, ffts_plus_task_def, window_id);

  vector<int64_t> output_addrs;
  if (!ge::AttrUtils::GetListInt(op_desc, "output_addrs", output_addrs)) {
    REPORT_FFTS_ERROR("[GenTsk][DataTsk][FillAutoCtxt][node %s, type %s]Attr output_addrs is empty.",
                      node->GetName().c_str(), node->GetType().c_str());
    return FAILED;
  }

  if (out_anchor_index >= output_addrs.size()) {
    REPORT_FFTS_ERROR("[GenTsk][DataTsk][FillAutoCtxt][node %s, type %s]out anchor %zu is >= the size of output_addrs \
                      %zu.", node->GetName().c_str(), node->GetType().c_str(), out_anchor_index, output_addrs.size());
    return FAILED;
  }
  data_ctx_def->set_addr_base(output_addrs[out_anchor_index]);

  ThreadSliceMapPtr slice_info_ptr = nullptr;
  slice_info_ptr = op_desc->TryGetExtAttr(kAttrSgtStructInfo, slice_info_ptr);
  FFTS_CHECK_NOTNULL(slice_info_ptr);
  uint32_t slice_num = slice_info_ptr->slice_instance_num;
  data_ctx_def->set_thread_dim(slice_info_ptr->slice_instance_num);
  data_ctx_def->set_thread_id(window_id);
  FillAutoThreadingParam(params, data_ctx_def, slice_num);
  FFTS_LOGI("[Auto mode]Finish gen %d context for node %s.", static_cast<int>(operation_), node->GetName().c_str());
  return SUCCESS;
}
}