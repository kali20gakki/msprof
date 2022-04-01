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

#include "aic_aiv_manual_task_builder.h"
#include "inc/ffts_utils.h"
#include "inc/graph/debug/ge_attr_define.h"

namespace ffts {
AICAIVTaskBuilder::AICAIVTaskBuilder() {}

AICAIVTaskBuilder::~AICAIVTaskBuilder() {}

Status AICAIVTaskBuilder::GenContextDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) {
  FFTS_LOGD("AIC AIV task builder genContextDef begin, node name:%s, node type:%s.", node->GetName().c_str(),
            node->GetType().c_str());
  auto op_desc = node->GetOpDesc();
  vector<FftsPlusComCtx_t> sub_ffts_plus_context;
  GenFftsPlusTaskCommonInfo(node, sub_ffts_plus_context);
  FftsPlusCtxDefPtr ctx_def_ptr = nullptr;
  ctx_def_ptr = op_desc->TryGetExtAttr(kAttrAICoreCtxDef, ctx_def_ptr);
  FFTS_CHECK_NOTNULL(ctx_def_ptr);
  domi::FftsPlusAicAivCtxDef *aicore_ctx_def = ctx_def_ptr->mutable_aic_aiv_ctx();
  FFTS_CHECK_NOTNULL(aicore_ctx_def);
  domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  FFTS_CHECK_NOTNULL(ffts_plus_ctx_def);
  std::string core_type;
  (void)ge::AttrUtils::GetStr(op_desc, ge::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type);
  if (core_type.empty()) {
    return FAILED;
  }
  if (core_type == kCoreTypeAIC) {
    ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_AICORE);
  } else {
    ffts_plus_ctx_def->set_context_type(RT_CTX_TYPE_AIV);
  }
  ffts_plus_ctx_def->set_op_index(op_desc->GetId());

  uint32_t context_id = 0;
  if (ge::AttrUtils::GetInt(op_desc, kContextId, context_id)) {
    ffts_plus_ctx_def->set_context_id(context_id);
  }
  FFTS_LOGD("GenContextDef nodetype:%s, name:%s, context_type:%u, op_index:%u", node->GetType().c_str(),
            node->GetName().c_str(), ffts_plus_ctx_def->context_type(), ffts_plus_ctx_def->op_index());
  domi::FftsPlusAicAivCtxDef *aic_aiv_ctx_def = ffts_plus_ctx_def->mutable_aic_aiv_ctx();
  FFTS_CHECK_NOTNULL(aic_aiv_ctx_def);
  FillContextData(aicore_ctx_def, aic_aiv_ctx_def);

  if (sub_ffts_plus_context.empty()) {
    return FAILED;
  }
  aic_aiv_ctx_def->set_pred_cnt(sub_ffts_plus_context[0].pred_cnt);
  aic_aiv_ctx_def->set_pred_cnt_init(sub_ffts_plus_context[0].pred_cnt);
  aic_aiv_ctx_def->set_aten(0);
  aic_aiv_ctx_def->set_successor_num(0);

  (void)ge::AttrUtils::SetListInt(op_desc, kSuccList, sub_ffts_plus_context[0].succ_list);

  uint32_t addr_size = aic_aiv_ctx_def->task_addr_size();
  uint32_t cur_addr_size = ffts_plus_task_def->addr_size();
  ffts_plus_task_def->set_addr_size(cur_addr_size + addr_size);
  FFTS_LOGD("GenContextDef nodetype:%s, name:%s, total_addr_size:%u", node->GetType().c_str(),
            node->GetName().c_str(), ffts_plus_task_def->addr_size());
  return SUCCESS;
}
}  // namespace ffts
