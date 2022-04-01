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

#include "deploy/flowrm/queue_schedule_manager.h"
#include <unistd.h>
#include <sys/prctl.h>
#include "common/debug/ge_log.h"
#include "ge/ge_api_error_codes.h"
#include "aicpu/queue_schedule/dgw_client.h"
#include "mmpa/mmpa_api.h"

namespace ge {
Status QueueScheduleManager::StartQueueSchedule(uint32_t device_id,
                                                uint32_t vf_id,
                                                const std::string &group_name,
                                                pid_t &pid) {
  auto qs_pid = fork();
  if (qs_pid < 0) {
    GELOGE(FAILED, "Fork host datagw server error.");
    return FAILED;
  }

  if (qs_pid == 0) {
    std::string str_device_id = std::string("--deviceId=") + std::to_string(device_id);
    std::string str_vf_id = std::string("--vfId=") + std::to_string(vf_id);
    std::string str_pid = std::string("--pid=") + std::to_string(getppid());
    std::string str_group_name = std::string("--qsInitGroupName=") + group_name;
    std::string path = "/usr/local/Ascend/runtime/bin/queue_schedule";
    const uint64_t sched_policy = static_cast<uint64_t>(bqs::SchedPolicy::POLICY_SUB_BUF_EVENT);
    std::string str_sched_policy = std::string("--schedPolicy=") + std::to_string(sched_policy);
    char *args[] = {const_cast<char *>(path.c_str()), const_cast<char *>(str_device_id.c_str()),
                    const_cast<char *>(str_vf_id.c_str()), const_cast<char *>(str_pid.c_str()),
                    const_cast<char *>(str_group_name.c_str()), const_cast<char *>(str_sched_policy.c_str()), NULL};
    ::prctl(PR_SET_PDEATHSIG, SIGKILL);
    int64_t res = execv(path.c_str(), args);
    if (res < 0) {
      GELOGE(FAILED, "Exec host datagw server failed.");
      REPORT_CALL_ERROR("E19999", "Exec host datagw server failed.");
      return FAILED;
    }

    return SUCCESS;
  }

  pid = qs_pid;
  return SUCCESS;
}

Status QueueScheduleManager::Shutdown(pid_t pid) {
  int32_t status = 0;
  if (kill(pid, SIGTERM) == 0) {
    GELOGI("kill queue schedule success, pid=%d", pid);
    const pid_t ret = mmWaitPid(pid, &status, 0);
    GELOGI("mmWaitPid queue schedule[%d] ret[%d] status[%d] finish", pid, ret, status);
    return SUCCESS;
  }

  GELOGI("queue schedule[%d] stopping", pid);
  uint32_t times = 0U;
  while (times < 10U) {
    const pid_t ret = mmWaitPid(pid, &status, M_WAIT_NOHANG);
    if (ret != 0) {
      GELOGI("mmWaitPid success, ret[%d] pid[%d] status[%d]", ret, pid, status);
      return SUCCESS;
    }
    (void) mmSleep(100U);
    ++times;
  }
  GELOGI("queue schedule[%d] stopping timeout, ready to kill", pid);
  if (kill(pid, SIGKILL) != 0) {
    const pid_t ret = mmWaitPid(pid, &status, 0);
    GELOGI("mmWaitPid queue schedule[%d] ret[%d] status[%d] finish", pid, ret, status);
  }
  GELOGI("queue schedule[%d] stopped", pid);
  return SUCCESS;
}
}  // namespace ge
