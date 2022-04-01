/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022. All rights reserved.
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

#include "graph/load/model_manager/task_info/cmo_barrier_task_info.h"
#include "graph/load/model_manager/davinci_model.h"

namespace ge {
Status CmoBarrierTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("Begin to init cmo barrier task info.");
  GE_CHECK_NOTNULL(davinci_model);
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model->GetStreamList()));

  const domi::CmoBarrierTaskDef &cmo_barrier_task_def = task_def.cmo_barrier_task();
  barrier_task_info_.logicIdNum = static_cast<uint8_t>(cmo_barrier_task_def.logic_id_num());
  const int32_t barrier_info_size = cmo_barrier_task_def.barrier_info_size();
  if (barrier_info_size > static_cast<int32_t>(RT_CMO_MAX_BARRIER_NUM)) {
    GELOGE(PARAM_INVALID, "Current barrier info size is %d, should not > %d", barrier_info_size,
           static_cast<int32_t>(RT_CMO_MAX_BARRIER_NUM));
    return PARAM_INVALID;
  }
  for (int32_t index = 0; index < barrier_info_size; ++index) {
    const domi::CmoBarrierInfoDef &barrier_info_def = cmo_barrier_task_def.barrier_info(index);
    barrier_task_info_.cmoInfo[index].cmoType = static_cast<uint16_t>(barrier_info_def.cmo_type());
    barrier_task_info_.cmoInfo[index].logicId = barrier_info_def.logic_id();
  }
  return SUCCESS;
}

Status CmoBarrierTaskInfo::Distribute() {
  GELOGI("Begin to distribute cmo barrier task info.");
  GE_CHK_RT_RET(rtBarrierTaskLaunch(&barrier_task_info_, stream_, 0U));
  GELOGI("Distribute cmo barrier task info sucess.");
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_BARRIER, CmoBarrierTaskInfo);
}  // namespace ge
