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

#include "deploy/flowrm/datagw_manager.h"
#include <sys/wait.h>
#include "deploy/flowrm/queue_schedule_manager.h"
#include "mmpa/mmpa_api.h"

namespace ge {
Status DataGwManager::InitHostDataGwServer(uint32_t device_id, uint32_t vf_id,
                                           const std::string &group_name, pid_t &dgw_pid) {
  GE_CHK_STATUS_RET_NOLOG(QueueScheduleManager::StartQueueSchedule(device_id, vf_id, group_name, dgw_pid));
  GE_CHK_STATUS_RET_NOLOG(ge::MemoryGroupManager::GetInstance().MemGrpAddProc(group_name, dgw_pid, false, true));
  GE_CHK_RT_RET(rtBufEventTrigger(group_name.c_str()));
  SetDataGwPid(device_id, dgw_pid);
  return SUCCESS;
}

Status DataGwManager::InitDeviceDataGwServer(uint32_t device_id, const std::string &group_name, pid_t &dgw_pid) {
  const auto sched_policy = static_cast<uint64_t>(bqs::SchedPolicy::POLICY_SUB_BUF_EVENT);
  const rtInitFlowGwInfo_t init_info = {group_name.c_str(), sched_policy};
  rtError_t ret = rtMemQueueInitFlowGw(device_id, &init_info);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "Init datagw server failed, device_id=%d, group name=%s.", device_id, group_name.c_str());
    REPORT_CALL_ERROR("E19999", "Init datagw server failed, device_id=%d, group name=%s.", device_id,
                      group_name.c_str());
    return FAILED;
  }
  GE_CHK_RT_RET(rtBufEventTrigger(group_name.c_str()));

  rtBindHostpidInfo_t info = {0};
  info.cpType = RT_DEV_PROCESS_QS;
  info.hostPid = getpid();
  info.chipId = device_id;
  pid_t pid = 0;
  ret = rtQueryDevPid(&info, &pid);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "Query datagw server failed, device_id=%d, group name=%s.", device_id, group_name.c_str());
    REPORT_CALL_ERROR("E19999", "Query datagw server failed, device_id=%d, group name=%s.", device_id,
                      group_name.c_str());
    return FAILED;
  }

  dgw_pid = pid;
  GE_CHK_STATUS_RET(ge::MemoryGroupManager::GetInstance().MemGrpAddProc(group_name, dgw_pid, false, true),
                    "Add datagw server pid[%d] into group[%s] failed.", dgw_pid, group_name.c_str());

  SetDataGwPid(device_id, dgw_pid);
  return SUCCESS;
}

Status DataGwManager::InitDataGwClient(uint32_t dgw_pid, uint32_t device_id) {
  std::string proc_sign = "";
  int32_t res = bqs::DgwClient::GetInstance(device_id)->Initialize(dgw_pid, proc_sign);
  if (res != 0) {
    GELOGE(FAILED, "Init datagw client failed, dgw server pid=%d, devie id=%d.", dgw_pid, device_id);
    REPORT_CALL_ERROR("E19999", "Init datagw client failed, dgw server pid=%d, devie id=%d.", dgw_pid, device_id);
    return FAILED;
  }
  return SUCCESS;
}

Status DataGwManager::CreateDataGwHandle(uint32_t device_id, bqs::CreateHcomInfo& hcom_info) {
  int32_t res = bqs::DgwClient::GetInstance(device_id)->CreateHcomHandle(hcom_info.masterIp, hcom_info.masterPort,
                                                                         hcom_info.localIp, hcom_info.localPort,
                                                                         hcom_info.remoteIp, hcom_info.remotePort,
                                                                         hcom_info.handle);
  if (res != 0) {
    GELOGE(FAILED, "Init datagw handle failed, master ip=%s, master port=%d, local ip=%s, local port=%d,"
           "remote ip=%s, remote port=%d,", hcom_info.masterIp, hcom_info.masterPort, hcom_info.localIp,
           hcom_info.localPort, hcom_info.remoteIp, hcom_info.remotePort);
    REPORT_CALL_ERROR("E19999", "Init datagw handle failed, master ip=%s, master port=%d, local ip=%s, local port=%d,"
                      "remote ip=%s, remote port=%d,", hcom_info.masterIp, hcom_info.masterPort, hcom_info.localIp,
                      hcom_info.localPort, hcom_info.remoteIp, hcom_info.remotePort);
    return FAILED;
  }
  return SUCCESS;
}

Status DataGwManager::CreateDataGwTag(uint64_t hcom_handle, const std::string &tag_name, uint32_t device_id,
                                      int32_t &hcom_tag) {
  int32_t res = bqs::DgwClient::GetInstance(device_id)->CreateHcomTag(hcom_handle, tag_name, hcom_tag);
  if (res != 0) {
    GELOGE(FAILED, "[Create][Tag] failed, hcom_handle=%lu, tag_name=%s, device id=%u, tag=%d.",
           hcom_handle, tag_name.c_str(), device_id, hcom_tag);
    REPORT_CALL_ERROR("E19999", "Create datagw tag failed, hcom_handle=%lu, tag name=%s, device id=%u",
                      hcom_handle, tag_name.c_str(), device_id);
    return FAILED;
  }
  GELOGD("[Create][Tag] success, hcom_handle=%lu, tag name=%s, device id=%u", hcom_handle, tag_name.c_str(), device_id);
  return SUCCESS;
}

Status DataGwManager::DestroyDataGwTag(uint32_t device_id, uint64_t hcom_handle, int32_t hcom_tag) {
  int32_t res = bqs::DgwClient::GetInstance(device_id)->DestroyHcomTag(hcom_handle, hcom_tag);
  if (res != 0) {
    GELOGE(FAILED, "[Destroy][Tag] failed, hcom_handle=%lu, hcom_tag=%d, device id=%u",
           hcom_handle, hcom_tag, device_id);
    REPORT_CALL_ERROR("E19999", "[Destroy][Tag] failed, hcom_handle=%lu, hcom_tag=%d, device id=%u",
                      hcom_handle, hcom_tag, device_id);
    return FAILED;
  }
  GELOGD("[Destroy][Tag] success, hcom_handle=%lu, hcom_tag=%d, device id=%u",
         hcom_handle, hcom_tag, device_id);
  return SUCCESS;
}

Status DataGwManager::CreateDataGwGroup(uint32_t device_id,
                                        const std::vector<bqs::Endpoint> &endpoint_list,
                                        int32_t &group_id) {
  if (endpoint_list.empty()) {
    GELOGE(FAILED, "[Create][Group] failed, endpoint list is empty, device id=%u.", device_id);
    REPORT_CALL_ERROR("E19999", "Create datagw group failed, endpoint list is empty, device id=%u.", device_id);
    return FAILED;
  }
  bqs::ConfigInfo cfg = {};
  cfg.cmd = bqs::ConfigCmd::DGW_CFG_CMD_ADD_GROUP;
  cfg.cfg.groupCfg.endpointNum = static_cast<uint32_t>(endpoint_list.size());
  cfg.cfg.groupCfg.endpoints = const_cast<bqs::Endpoint *>(&endpoint_list[0]);
  std::vector<int32_t> results;
  int32_t ret = bqs::DgwClient::GetInstance(device_id)->UpdateConfig(cfg, results);
  if (ret != 0) {
    GELOGE(FAILED, "[Create][Group] failed, ret = %d, endpoint type=%d, endpoint size=%zu, device id=%u.",
           ret, static_cast<int32_t>(endpoint_list[0].type), endpoint_list.size(), device_id);
    REPORT_CALL_ERROR("E19999", "Create datagw group failed, ret = %d, "
                      "endpoint type=%d, endpoint size=%zu, device id=%u.",
                      ret, static_cast<int32_t>(endpoint_list[0].type), endpoint_list.size(), device_id);
    return FAILED;
  }
  group_id = cfg.cfg.groupCfg.groupId;
  GELOGD("[Create][Group] success, endpoint type=%d, endpoint size=%zu, device id=%u.",
         static_cast<int32_t>(endpoint_list[0].type), endpoint_list.size(), device_id);
  return SUCCESS;
}

Status DataGwManager::DestroyDataGwGroup(uint32_t device_id, int32_t group_id) {
  bqs::ConfigInfo cfg = {};
  cfg.cmd = bqs::ConfigCmd::DGW_CFG_CMD_DEL_GROUP;
  cfg.cfg.groupCfg.groupId = group_id;
  std::vector<int32_t> results;
  int32_t ret = bqs::DgwClient::GetInstance(device_id)->UpdateConfig(cfg, results);
  if (ret != 0) {
    GELOGE(FAILED, "[Destroy][Group] failed, ret = %d, group_id=%d, device id=%u.", ret, group_id, device_id);
    REPORT_CALL_ERROR("E19999", "Destroy datagw group failed, ret = %d, group_id=%d, device id=%u.",
                      ret, group_id, device_id);
    return FAILED;
  }
  GELOGD("[Destroy][Group] success, group_id=%d, device id=%u.", group_id, device_id);
  return SUCCESS;
}

Status DataGwManager::GrantQueueForDataGw(uint32_t device_id, const bqs::Route &queue_route) {
  GELOGD("Grant queue for datagw, src type=%d, dst type=%d.",
         static_cast<int32_t>(queue_route.src.type), static_cast<int32_t>(queue_route.dst.type));

  if ((queue_route.src.type != bqs::EndpointType::QUEUE) && (queue_route.dst.type != bqs::EndpointType::QUEUE)) {
    GELOGI("src queue and dst queue are both no need to grant.");
    return SUCCESS;
  }

  int32_t dgw_pid = GetDataGwPid(device_id);
  if (queue_route.src.type == bqs::EndpointType::QUEUE) {
    GE_CHK_STATUS_RET(GrantQueue(device_id, queue_route.src.attr.queueAttr.queueId, dgw_pid, READ_ONLY),
                      "Grant src queue failed, device id=%u, src queue id=%d, datagw pid = %d",
                      device_id, queue_route.src.attr.queueAttr.queueId, dgw_pid);
  }

  if (queue_route.dst.type == bqs::EndpointType::QUEUE) {
    GE_CHK_STATUS_RET(GrantQueue(device_id, queue_route.dst.attr.queueAttr.queueId, dgw_pid, WRITE_ONLY),
                      "Grant dst queue failed, device id=%u, src queue id=%d, datagw pid = %d",
                      device_id, queue_route.dst.attr.queueAttr.queueId, dgw_pid);
  }
  return SUCCESS;
}

Status DataGwManager::GrantQueueForCpuSchedule(uint32_t device_id, int32_t pid,
                                               const std::vector<uint32_t> &input_queues,
                                               const std::vector<uint32_t> &output_queues) {
  GELOGD("Grant queue for aicpu schedule, device = %u, pid = %d", device_id, pid);
  for (const uint32_t queue_id : input_queues) {
    GE_CHK_STATUS_RET(GrantQueue(device_id, queue_id, pid, READ_AND_WRITE),
                      "Grant src queue failed, device id=%u, src queue id=%u, aicpu pid = %d",
                      device_id, queue_id, pid);
  }

  for (const uint32_t queue_id : output_queues) {
    GE_CHK_STATUS_RET(GrantQueue(device_id, queue_id, pid, READ_AND_WRITE),
                      "Grant src queue failed, device id=%u, src queue id=%u, aicpu pid = %d",
                      device_id, queue_id, pid);
  }

  return SUCCESS;
}

Status DataGwManager::BindQueues(uint32_t device_id, const std::vector<bqs::Route> &queue_routes) {
  GELOGD("[Bind][Queues] device_id = %u, routes size = %zu", device_id, queue_routes.size());
  if (queue_routes.empty()) { return SUCCESS; }
  for (auto &queue_route : queue_routes) {
    GE_CHK_STATUS_RET(GrantQueueForDataGw(device_id, queue_route),
                      "[Grant][Queue] failed, src queue id=%d, src type=%d, dst queue id=%d, dst type=%d.",
                      queue_route.src.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.src.type),
                      queue_route.dst.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.dst.type));
  }

  bqs::ConfigInfo cfg = {};
  cfg.cmd = bqs::ConfigCmd::DGW_CFG_CMD_BIND_ROUTE;
  cfg.cfg.routesCfg.routeNum = queue_routes.size();
  cfg.cfg.routesCfg.routes = const_cast<bqs::Route *>(queue_routes.data());
  std::vector<int32_t> bind_results;
  int32_t ret = bqs::DgwClient::GetInstance(device_id)->UpdateConfig(cfg, bind_results);
  if (ret != 0) {
    for (auto &queue_route : queue_routes) {
      GELOGE(FAILED, "[Bind][Route] failed, ret = %d, src queue id=%d, src type=%d, dst queue id=%d, dst type=%d.",
             ret, queue_route.src.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.src.type),
             queue_route.dst.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.dst.type));
    }

    REPORT_CALL_ERROR("E19999", "Bind queues failed, ret = %d, device_id = %u, routes size = %zu",
                      ret, device_id, queue_routes.size());
    return FAILED;
  }
  for (auto &queue_route : queue_routes) {
    GELOGD("[Bind][Route] success, src queue id=%d, src type=%d, dst queue id=%d, dst type=%d.",
           queue_route.src.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.src.type),
           queue_route.dst.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.dst.type));
  }
  return SUCCESS;
}

Status DataGwManager::UnbindQueues(uint32_t device_id, const bqs::Route &queue_route) {
  bqs::ConfigInfo cfg = {};
  cfg.cmd = bqs::ConfigCmd::DGW_CFG_CMD_UNBIND_ROUTE;
  cfg.cfg.routesCfg.routeNum = 1U;
  cfg.cfg.routesCfg.routes = const_cast<bqs::Route *>(&queue_route);
  std::vector<int32_t> results;
  int32_t ret = bqs::DgwClient::GetInstance(device_id)->UpdateConfig(cfg, results);
  if (ret != 0) {
    GELOGE(FAILED, "[Unbind][Route] failed, ret = %d, src queue id=%d, src type=%d, dst queue id=%d, dst type=%d.",
           ret, queue_route.src.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.src.type),
           queue_route.dst.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.dst.type));
    REPORT_CALL_ERROR("E19999", "Unbind queue failed, ret = %d, src queue id=%u, src type=%u, "
                      "dst queue id=%u, dst type=%u.",
                      ret, queue_route.src.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.src.type),
                      queue_route.dst.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.dst.type));
    return FAILED;
  }
  GELOGD("[Unbind][Route] success, src queue id=%d, src type=%d, dst queue id=%d, dst type=%d.",
         queue_route.src.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.src.type),
         queue_route.dst.attr.queueAttr.queueId, static_cast<int32_t>(queue_route.dst.type));
  return SUCCESS;
}

Status DataGwManager::GrantQueue(uint32_t device_id, uint32_t qid, pid_t pid, GrantType grant_type) {
  rtMemQueueShareAttr_t attr = {0};
  if (grant_type == READ_ONLY) {
    attr.read = 1;
  } else if (grant_type == WRITE_ONLY) {
    attr.write = 1;
  } else if (grant_type == READ_AND_WRITE) {
    attr.read = 1;
    attr.write = 1;
  } else {
    GELOGE(FAILED, "[Grant][Queue] type[%d] error.", grant_type);
    REPORT_INNER_ERROR("E19999", "Grant queue type[%d] error.", grant_type);
    return FAILED;
  }

  rtError_t ret = rtMemQueueGrant(device_id, qid, pid, &attr);
  if (ret != RT_ERROR_NONE) {
    GELOGE(FAILED, "[Grant][Queue] queue[%u] to pid[%d] on device_id[%u] error, type=%d", qid, pid, device_id,
           grant_type);
    REPORT_CALL_ERROR("E19999", "Grant queue[%u] to pid[%d] on device_id[%u] error, type=%d", qid, pid,
                      device_id, grant_type);
    return FAILED;
  }
  return SUCCESS;
}

Status DataGwManager::InitQueue(uint32_t device_id) {
  rtError_t ret = rtMemQueueInit(static_cast<int32_t>(device_id));
  if (ret != RT_ERROR_NONE) {
    GELOGW("On device[%u], init queue failed.", device_id);
  }
  return SUCCESS;
}

Status DataGwManager::Shutdown() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (dgw_pid_map_.empty()) {
    GELOGD("queue schedule not start.");
    return SUCCESS;
  }

  for (auto it = dgw_pid_map_.begin(); it != dgw_pid_map_.end(); ++it) {
    if (it->second != 0) {
      GE_CHK_STATUS(QueueScheduleManager::Shutdown(it->second));
    }
  }
  dgw_pid_map_.clear();
  return SUCCESS;
}

Status DataGwManager::DestroyDataGwHandle(uint32_t device_id, uint64_t hcom_handle) {
  int32_t res = bqs::DgwClient::GetInstance(device_id)->DestroyHcomHandle(hcom_handle);
  if (res != 0) {
    GELOGE(FAILED, "[Destroy][Handle] failed, hcom_handle=%lu, device id=%u", hcom_handle, device_id);
    REPORT_CALL_ERROR("E19999", "[Destroy][Handle] failed, hcom_handle=%lu, device id=%u", hcom_handle, device_id);
    return FAILED;
  }
  GELOGD("[Destroy][Handle] success, hcom_handle=%lu, device id=%u", hcom_handle, device_id);
  return SUCCESS;
}

void DataGwManager::SetDataGwPid(uint32_t device_id, int32_t dwg_pid) {
  std::lock_guard<std::mutex> lock(mutex_);
  dgw_pid_map_[device_id] = dwg_pid;
}

int32_t DataGwManager::GetDataGwPid(uint32_t device_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  return dgw_pid_map_[device_id];
}
}
