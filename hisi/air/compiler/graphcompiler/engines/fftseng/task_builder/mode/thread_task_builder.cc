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

#include "task_builder/mode/thread_task_builder.h"
#include "common/sgt_slice_type.h"
#include "inc/graph/debug/ge_attr_define.h"

namespace ffts {
TheadTaskBuilder::TheadTaskBuilder() {}

TheadTaskBuilder::~TheadTaskBuilder() {}

void TheadTaskBuilder::SetModeType(const ModeType &type) {
  mode_type_ = type;
}

Status TheadTaskBuilder::GetNodeContextTypeByNode(const ge::NodePtr &node, TaskBuilderType &task_builder_type) const {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  if (op_desc->HasAttr(kAttrAICoreCtxType)) {
    int64_t ctx_type = static_cast<int64_t>(TaskBuilderType::EN_TASK_TYPE_RESERVED);
    (void)ge::AttrUtils::GetInt(op_desc, kAttrAICoreCtxType, ctx_type);
    task_builder_type = TaskBuilderType(ctx_type);
    return SUCCESS;
  }

  if (kHCCLOpType.count(op_desc->GetType()) > 0) {
    task_builder_type = TaskBuilderType::EN_TASK_TYPE_COLLECTION_COMMICATE;
    return SUCCESS;
  }

  FftsPlusCtxDefPtr ctx_def_ptr = nullptr;
  ctx_def_ptr = op_desc->TryGetExtAttr("_ffts_plus_aicpu_ctx_def", ctx_def_ptr);
  if (ctx_def_ptr != nullptr) {
    ffts::ThreadSliceMapPtr slice_info_ptr = nullptr;
    slice_info_ptr = op_desc->TryGetExtAttr(ffts::kAttrSgtStructInfo, slice_info_ptr);
    FFTS_CHECK_NOTNULL(slice_info_ptr);
    if (slice_info_ptr->thread_mode == static_cast<uint32_t>(ThreadMode::AUTO_THREAD)) {
      task_builder_type = TaskBuilderType::EN_TASK_TYPE_AICPU_AUTO;
    } else {
      task_builder_type = TaskBuilderType::EN_TASK_TYPE_AICPU;
    }
    return SUCCESS;
  }

  FftsPlusCtxDefPtr runtime_control_ctx_def =  nullptr;
  runtime_control_ctx_def = op_desc->TryGetExtAttr("FFTS_PLUS_TASK_DEF", runtime_control_ctx_def);
  if (runtime_control_ctx_def != nullptr) {
    task_builder_type = TaskBuilderType::EN_TASK_TYPE_RUNTIME_CONTROL;
    return SUCCESS;
  }

  return FAILED;
}

FFTSPlusTaskBuilderPtr TheadTaskBuilder::GetTaskBuilder(TaskBuilderType task_builder_type) {
  switch (task_builder_type) {
    case TaskBuilderType::EN_TASK_TYPE_AIC_AIV:
      return aic_aiv_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_AIC_AIV_AUTO:
     return aic_aiv_auto_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_AIC_AIV_DYNAMIC:
      return aic_aiv_dynamic_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV:
      return mix_aic_aiv_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_MIX_AIC_AIV_AUTO:
      return mix_aic_aiv_auto_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_MIX_L2_AIC_AIV:
      return mix_L2_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_COLLECTION_COMMICATE:
      return collection_ops_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_AICPU:
      return aicpu_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_AICPU_AUTO:
      return aicpu_auto_task_builder_ptr_;
    case TaskBuilderType::EN_TASK_TYPE_RUNTIME_CONTROL:
      return runtime_ops_task_builder_ptr_;
    default:
      return nullptr;
  }
}

Status TheadTaskBuilder::GenerateDataTaskDef(const ge::NodePtr &node, domi::FftsPlusTaskDef *ffts_plus_task_def,
                                             const ModeType &mode_type) const {
  FFTS_LOGD("Current ffts plus mode type is : %d.", static_cast<int>(mode_type));
  if (mode_type == ModeType::MANUAL_MODE_TYPE) {
    PrefetchTaskBuilder prefetch;
    OutTaskBuilder invalid(CACHE_OPERATION::INVALIDATE);
    OutTaskBuilder write_back(CACHE_OPERATION::WRITE_BACK);
    if (prefetch.GenManualDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
    if (invalid.GenManualDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
    if (write_back.GenManualDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
  } else if (mode_type == ModeType::AUTO_MODE_TYPE) {
    PrefetchAutoTaskBuilder prefetch_auto;
    OutAutoTaskBuilder invalid_auto(CACHE_OPERATION::INVALIDATE);
    OutAutoTaskBuilder write_back_auto(CACHE_OPERATION::WRITE_BACK);
    if (prefetch_auto.GenAutoDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
    if (invalid_auto.GenAutoDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
    if (write_back_auto.GenAutoDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
  } else if (mode_type == ModeType::DYNAMIC_MODE_TYPE) {
    PrefetchDynamicTaskBuilder prefetch_dyn;
    OutDynamicTaskBuilder invalid_dyn(CACHE_OPERATION::INVALIDATE);
    OutDynamicTaskBuilder write_back_dyn(CACHE_OPERATION::WRITE_BACK);
    if (prefetch_dyn.GenDynamicDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
    if (invalid_dyn.GenDynamicDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
    if (write_back_dyn.GenDynamicDataCtxDef(node, ffts_plus_task_def) != SUCCESS) {
      return FAILED;
    }
  }
  return SUCCESS;
}

bool TheadTaskBuilder::IsNoCtx(const ge::NodePtr &node) {
  ge::OpDescPtr op_desc = node->GetOpDesc();
  if (NO_NEED_GEN_TASK_OP_TYPE.count(op_desc->GetType()) != 0) {
    return true;
  }
  bool no_task = false;
  (void)ge::AttrUtils::GetBool(op_desc, ge::ATTR_NAME_NOTASK, no_task);
  if (no_task) {
    return true;
  }
  return false;
}
}  // namespace ffts
