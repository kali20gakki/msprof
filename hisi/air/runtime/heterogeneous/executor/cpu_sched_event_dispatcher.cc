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

#include "executor/cpu_sched_event_dispatcher.h"
#include "mmpa/mmpa_api.h"
#include "common/utils/rts_api_utils.h"
#include "executor/dynamic_model_executor.h"
#include "runtime/rt_mem_queue.h"
#include "external/runtime/rt_error_codes.h"
#include "aicpu/aicpu_schedule/aicpusd_interface.h"

namespace ge {
namespace {
uint32_t kAiCpuSubEventIdEndGraph = 6U;
uint32_t kAiCpuSubEventIdActivateModel = 7U;
}  // namespace

CpuSchedEventDispatcher::~CpuSchedEventDispatcher() {
  if (running_) {
    Finalize();
  }
}

Status CpuSchedEventDispatcher::Initialize(int32_t device_id) {
  device_id_ = device_id;
  aicpu_sd_pid_ = mmGetPid();
  GE_CHK_STATUS_RET(RtsApiUtils::EschedCreateGroup(device_id_, event_group_id_, RT_GRP_TYPE_BIND_CP_CPU),
                    "Failed to create group, device_id = %d", device_id);
  uint64_t event_bitmap = 1ULL << static_cast<uint32_t>(RT_EVENT_AICPU_MSG);
  GE_CHK_STATUS_RET(RtsApiUtils::EschedSubscribeEvent(device_id_, event_group_id_, 0, event_bitmap),
                    "Failed to subscribe event, device_id = %u", device_id_);
  CpuSchedInitParam init_param{};
  init_param.deviceId = device_id_;
  init_param.hostPid = aicpu_sd_pid_;
  init_param.profilingMode = PROFILING_CLOSE;
  auto ret = InitCpuScheduler(&init_param);
  GE_CHK_BOOL_RET_STATUS(ret == 0, FAILED, "Failed to invoke InitHostAICPUScheduler, ret = %d", ret);
  running_ = true;
  event_handle_thread_ = std::thread([this]() {
    this->ProcessEvents();
  });
  return SUCCESS;
}

Status CpuSchedEventDispatcher::OnInputsReady(rtEschedEventSummary_t &in_event) {
  if (in_event.msgLen < sizeof(AICPUSubEventInfo)) {
    GELOGE(PARAM_INVALID, "event msg length is insufficient for AICPUSubEventInfo, msgLen = %u", in_event.msgLen);
    return PARAM_INVALID;
  }

  auto model_id = reinterpret_cast<AICPUSubEventInfo *>(in_event.msg)->modelId;
  GELOGD("On activate model event, model_id = %u", model_id);

  std::lock_guard<std::mutex> lk(mu_);
  const auto &it = models_.find(model_id);
  if (it == models_.end()) {
    GELOGW("model id not found, id = %u", model_id);
    return SUCCESS;
  }

  auto callback = [model_id, this](Status status) {
    OnModelExecuted(model_id, status);
    if (status != SUCCESS) {
      GELOGE(FAILED, "Execute model failed, model_id = %u", model_id);
      running_ = false;
    }
  };

  auto &model_executor = *it->second;
  GE_CHK_STATUS_RET(model_executor.ExecuteAsync(callback), "Failed to submit task, model id = %u", model_id);
  GELOGD("Activate model success, model_id = %u", model_id);
  return SUCCESS;
}

void CpuSchedEventDispatcher::ProcessEvents() {
  GELOGI("Process thread started");
  const int32_t timeout = 10 * 1000;
  while (running_) {
    rtEschedEventSummary_t in_event;
    const auto ret = rtEschedWaitEvent(device_id_, event_group_id_, 0, timeout, &in_event);
    if (ret == ACL_ERROR_RT_REPORT_TIMEOUT) {
      GELOGI("wait timeout, continue");
      continue;
    }
    if (ret != RT_ERROR_NONE) {
      GELOGE(RT_FAILED, "Failed to invoke rtEschedWaitEvent, device_id = %d, group_id = %u, ret = 0x%X",
             device_id_, event_group_id_, ret);
      running_ = false;
      return;
    }
    if (in_event.eventId != RT_EVENT_AICPU_MSG || in_event.subeventId != kAiCpuSubEventIdActivateModel) {
      GELOGD("Ignore event, event_id = %d, sub_event_id = %u", in_event.eventId,  in_event.subeventId);
      continue;
    }

    if (OnInputsReady(in_event) != SUCCESS) {
      running_ = false;
      return;
    }
  }
  GELOGI("Process thread ended");
}

Status CpuSchedEventDispatcher::OnModelExecuted(uint32_t model_id, Status status) const {
  // notify aicpu-sd
  (void) model_id;
  GELOGD("Notify model execution ended, model_id = %u, status = %u", model_id, status);
  AICPUSubEventInfo sub_event_info{};
  sub_event_info.modelId = model_id;
  sub_event_info.para.endGraphInfo.result = status;

  rtEschedEventSummary_t event_info{};
  event_info.eventId = RT_EVENT_AICPU_MSG;
  event_info.pid = aicpu_sd_pid_;
  event_info.grpId = 0U;  // aicpu event group
  event_info.subeventId = kAiCpuSubEventIdEndGraph;  // AICPU_SUB_EVENT_END_GRAPH
  event_info.msg = reinterpret_cast<char *>(&sub_event_info);
  event_info.msgLen = sizeof(sub_event_info);
  GE_CHK_STATUS_RET(RtsApiUtils::EschedSubmitEvent(0, event_info),
                    "[Send][Event] failed, device_id = %d", device_id_);
  GELOGD("[Send][Event] succeeded, device_id = %d", device_id_);
  return SUCCESS;
}

void CpuSchedEventDispatcher::Finalize() {
  running_ = false;
  if (event_handle_thread_.joinable()) {
    event_handle_thread_.join();
  }
}

void CpuSchedEventDispatcher::Register(uint32_t model_id, DynamicModelExecutor *executor) {
  std::lock_guard<std::mutex> lk(mu_);
  models_[model_id] = executor;
}

void CpuSchedEventDispatcher::Deregister(uint32_t model_id) {
  std::lock_guard<std::mutex> lk(mu_);
  models_.erase(model_id);
}
}  // namespace ge
