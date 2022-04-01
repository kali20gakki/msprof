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

#include "graph/load/model_manager/task_info/cmo_task_info.h"
#include "graph/load/model_manager/davinci_model.h"
#include "graph/load/model_manager/model_utils.h"

namespace ge {
Status CmoTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("Begin to init cmo task info.");
  GE_CHECK_NOTNULL(davinci_model);
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model->GetStreamList()));

  const domi::CmoTaskDef &cmo_task_def = task_def.cmo_task();
  cmo_task_info_.cmoType = static_cast<uint16_t>(cmo_task_def.cmo_type());
  cmo_task_info_.logicId = cmo_task_def.logic_id();
  cmo_task_info_.qos = static_cast<uint8_t>(cmo_task_def.qos());
  cmo_task_info_.partId = static_cast<uint8_t>(cmo_task_def.part_id());
  cmo_task_info_.pmg = static_cast<uint8_t>(cmo_task_def.pmg());
  cmo_task_info_.opCode = static_cast<uint16_t>(cmo_task_def.op_code());
  cmo_task_info_.numInner = static_cast<uint16_t>(cmo_task_def.num_inner());
  cmo_task_info_.numOuter = static_cast<uint16_t>(cmo_task_def.num_outer());
  cmo_task_info_.lengthInner = cmo_task_def.length_inner();
  const auto &rts_param = davinci_model->GetRuntimeParam();
  uint8_t *source_mem_addr = nullptr;
  if (ModelUtils::GetRtAddress(rts_param, cmo_task_def.source_addr(), source_mem_addr) != SUCCESS) {
    GELOGE(INTERNAL_ERROR, "[Check][GetRtAddress]failed, logic write addr base is 0x%lx.", cmo_task_def.source_addr());
    return INTERNAL_ERROR;
  }
  cmo_task_info_.sourceAddr = PtrToValue(source_mem_addr);
  cmo_task_info_.striderOuter = cmo_task_def.strider_outer();
  cmo_task_info_.striderInner = cmo_task_def.strider_inner();
  GELOGI("cmoType[%u], logicId[%u], qos[%u], partid[%u], pmg[%u], opCode[%u], numInner[%u], numOuter[%u], "
         "lengthInner[%u], striderOuter[%u], striderInner[%u]", cmo_task_def.cmo_type(), cmo_task_def.logic_id(),
         cmo_task_def.qos(), cmo_task_def.part_id(), cmo_task_def.pmg(), cmo_task_def.op_code(),
         cmo_task_def.num_inner(), cmo_task_def.num_outer(), cmo_task_def.length_inner(), cmo_task_def.strider_outer(),
         cmo_task_def.strider_inner());
  return SUCCESS;
}

Status CmoTaskInfo::Distribute() {
  GELOGI("Begin to distribute cmo task info.");
  GE_CHK_RT_RET(rtCmoTaskLaunch(&cmo_task_info_, stream_, 0U));
  GELOGI("Distribute cmo task info sucess.");
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_CMO, CmoTaskInfo);
}  // namespace ge
