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

#include "mix_aic_aiv_manual_task_builder.h"
#include "inc/ffts_utils.h"
#include "inc/graph/debug/ge_attr_define.h"


namespace ffts {
static const vector<std::string> kMixPrefixs = { "_mix_aic", "_mix_aiv" };

MixAICAIVTaskBuilder::MixAICAIVTaskBuilder() {}

MixAICAIVTaskBuilder::~MixAICAIVTaskBuilder() {}

Status MixAICAIVTaskBuilder::GenContextDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def) {
  FFTS_LOGD("MixAICAIVTaskBuilder::GenContextDef begin, node name:%s, node type:%s.", node->GetName().c_str(),
            node->GetType().c_str());
  auto op_desc = node->GetOpDesc();
  vector<FftsPlusComCtx_t> sub_ffts_plus_context;
  GenFftsPlusTaskCommonInfo(node, sub_ffts_plus_context);
  FftsPlusCtxDefPtr ctx_def_ptr = nullptr;
  ctx_def_ptr = op_desc->TryGetExtAttr(kAttrAICoreCtxDef, ctx_def_ptr);
  FFTS_CHECK_NOTNULL(ctx_def_ptr);
  domi::FftsPlusMixAicAivCtxDef *aicore_ctx_def = ctx_def_ptr->mutable_mix_aic_aiv_ctx();
  FFTS_CHECK_NOTNULL(aicore_ctx_def);
  domi::FftsPlusCtxDef *ffts_plus_ctx_def = ffts_plus_task_def->add_ffts_plus_ctx();
  FFTS_CHECK_NOTNULL(ffts_plus_ctx_def);

  string core_type;
  rtFftsPlusContextType_t ctx_type = RT_CTX_TYPE_MIX_AIC;
  (void)ge::AttrUtils::GetStr(op_desc, ge::ATTR_NAME_CUBE_VECTOR_CORE_TYPE, core_type);
  if (core_type == kCoreTypeMixAIV) {
    ctx_type = RT_CTX_TYPE_MIX_AIV;
  }
  ffts_plus_ctx_def->set_context_type(ctx_type);
  ffts_plus_ctx_def->set_op_index(op_desc->GetId());
  uint32_t context_id = 0;
  if (ge::AttrUtils::GetInt(op_desc, kContextId, context_id)) {
    ffts_plus_ctx_def->set_context_id(context_id);
  }
  FFTS_LOGD("GenContextDef nodetype:%s, name:%s, context_type:%u, op_index:%u", node->GetType().c_str(),
            node->GetName().c_str(), ffts_plus_ctx_def->context_type(), ffts_plus_ctx_def->op_index());
  domi::FftsPlusMixAicAivCtxDef *mix_aic_aiv_ctx_def = ffts_plus_ctx_def->mutable_mix_aic_aiv_ctx();
  FFTS_CHECK_NOTNULL(mix_aic_aiv_ctx_def);

  FillContextData(aicore_ctx_def, mix_aic_aiv_ctx_def);

  if (sub_ffts_plus_context.empty()) {
    return FAILED;
  }
  mix_aic_aiv_ctx_def->set_pred_cnt(sub_ffts_plus_context[0].pred_cnt);
  mix_aic_aiv_ctx_def->set_pred_cnt_init(sub_ffts_plus_context[0].pred_cnt);
  mix_aic_aiv_ctx_def->set_aten(0);
  mix_aic_aiv_ctx_def->set_successor_num(0);

  (void)ge::AttrUtils::SetListInt(op_desc, kSuccList, sub_ffts_plus_context[0].succ_list);
  uint32_t addr_size = mix_aic_aiv_ctx_def->task_addr_size();
  uint32_t cur_addr_size = ffts_plus_task_def->addr_size();
  ffts_plus_task_def->set_addr_size(cur_addr_size + addr_size);
  FFTS_LOGD("GenContextDef nodetype:%s, name:%s, total_addr_size:%u", node->GetType().c_str(),
            node->GetName().c_str(), ffts_plus_task_def->addr_size());
  if (AddAdditionalArgs(op_desc, ffts_plus_task_def, 1) != SUCCESS) {
    REPORT_FFTS_ERROR("[MixAICAIVTaskBuilder] [AddAdditionalArgs] Add node[%s] additional args failed.",
                      op_desc->GetName().c_str());
    return FAILED;
  }

  return SUCCESS;
}

Status MixAICAIVTaskBuilder::AddAdditionalArgs(ge::OpDescPtr &op_desc, domi::FftsPlusTaskDef *ffts_plus_task_def,
                                               const size_t &ctx_num) const {
  FFTS_CHECK_NOTNULL(op_desc);
  // modeInArgsFirstField
  uint32_t mode = 0;
  (void) ge::AttrUtils::GetInt(op_desc, kModeInArgsFirstField, mode);
  if (mode == 1) {
    uint32_t data_type = 0;
    domi::AdditionalDataDef *additional_data_def = ffts_plus_task_def->add_additional_data();
    FFTS_CHECK_NOTNULL(additional_data_def);
    uint32_t context_id;
    (void) ge::AttrUtils::GetInt(op_desc, kContextId, context_id);
    additional_data_def->set_data_type(data_type);
    additional_data_def->add_context_id(context_id);
    if (ctx_num > 1) {
      (void) ge::AttrUtils::GetInt(op_desc, "_default_context_id", context_id);
      additional_data_def->add_context_id(context_id);
    }
  }
  return SUCCESS;
}
}  // namespace ffts
