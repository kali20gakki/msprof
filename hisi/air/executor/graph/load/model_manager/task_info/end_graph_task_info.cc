/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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

#include "graph/load/model_manager/task_info/end_graph_task_info.h"

#include "graph/load/model_manager/davinci_model.h"

namespace {
const uint32_t kDumpFlag = 2U;
}  // namespace
namespace ge {
Status EndGraphTaskInfo::Init(const domi::TaskDef &task_def, DavinciModel *const davinci_model) {
  GELOGI("InitEndGraphTaskInfo Init Start.");
  GE_CHECK_NOTNULL(davinci_model);
  davinci_model_ = davinci_model;
  GE_CHK_STATUS_RET_NOLOG(SetStream(task_def.stream_id(), davinci_model_->GetStreamList()));

  model_ = davinci_model_->GetRtModelHandle();
  GELOGI("InitEndGraphTaskInfo Init Success, model:%p, stream:%p", model_, stream_);
  return SUCCESS;
}

Status EndGraphTaskInfo::Distribute() {
  GELOGI("EndGraphTaskInfo Distribute Start.");
  GE_CHECK_NOTNULL(davinci_model_);
  rtError_t rt_ret;
  if (davinci_model_->ModelNeedDump()) {
    GELOGI("Start to call rtEndGraphEx");
    rt_ret = rtEndGraphEx(model_, stream_, kDumpFlag);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtEndGraphEx failed, ret:0x%X", rt_ret);
      GELOGE(RT_FAILED, "[Call][RtEndGraphEx] failed, ret:0x%x", rt_ret);
      return RT_ERROR_TO_GE_STATUS(rt_ret);
    }
  } else {
    GELOGI("Start to call rtEndGraph");
    rt_ret = rtEndGraph(model_, stream_);
    if (rt_ret != RT_ERROR_NONE) {
      REPORT_CALL_ERROR("E19999", "Call rtEndGraph failed, ret:0x%X", rt_ret);
      GELOGE(RT_FAILED, "[Call][RtEndGraph] failed, ret:0x%x", rt_ret);
      return RT_ERROR_TO_GE_STATUS(rt_ret);
    }
  }

  rt_ret = rtModelGetTaskId(davinci_model_->GetRtModelHandle(), &task_id_, &stream_id_);
  if (rt_ret != RT_ERROR_NONE) {
    REPORT_CALL_ERROR("E19999", "Call rtModelGetTaskId failed, ret:0x%X", rt_ret);
    GELOGE(RT_FAILED, "[Call][RtModelGetTaskId] failed, ret:0x%X", rt_ret);
    return RT_ERROR_TO_GE_STATUS(rt_ret);
  }
  davinci_model_->SetEndGraphId(task_id_, stream_id_);

  GELOGI("EndGraphTaskInfo Distribute Success, task id is %u, stream id is %u", task_id_, stream_id_);
  return SUCCESS;
}

REGISTER_TASK_INFO(RT_MODEL_TASK_MODEL_END_GRAPH, EndGraphTaskInfo);
}  // namespace ge
