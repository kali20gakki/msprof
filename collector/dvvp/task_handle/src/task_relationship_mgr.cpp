/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: manage relationships of task info
 * Author: zcj
 * Create: 2020-09-26
 */
#include "task_relationship_mgr.h"

#include "errno/error_code.h"
#include "msprof_dlog.h"


namespace Analysis {
namespace Dvvp {
namespace TaskHandle {
void TaskRelationshipMgr::AddHostIdDevIdRelationship(int hostId, int devId)
{
    MSPROF_LOGI("hostId: %d, devId: %d Entering HostId DeviceId Map...", hostId, devId);
    std::lock_guard<std::mutex> lock(hostIdMapMutex_);
    hostIdToDevId_[hostId] = devId;
}

int TaskRelationshipMgr::GetDevIdByHostId(int hostId)
{
    std::lock_guard<std::mutex> lock(hostIdMapMutex_);
    auto iter = hostIdToDevId_.find(hostId);
    if (iter != hostIdToDevId_.end()) {
        return iter->second;
    }
    return hostId;
}

int TaskRelationshipMgr::GetHostIdByDevId(int devId)
{
    std::lock_guard<std::mutex> lock(hostIdMapMutex_);
    for (auto iter = hostIdToDevId_.begin(); iter != hostIdToDevId_.end(); iter++) {
        if (iter->second == devId) {
            return iter->first;
        }
    }
    return devId;
}

void TaskRelationshipMgr::ResetDeviceIdHostId()
{
    MSPROF_LOGI("Reset host id - device id relationship");
    std::lock_guard<std::mutex> lock(hostIdMapMutex_);
    hostIdToDevId_.clear();
}

void TaskRelationshipMgr::AddLocalFlushJobId(const std::string &jobId)
{
    MSPROF_LOGI("Job %s should flush locally", jobId.c_str());
    localFlushJobIds_.insert(jobId);
}

void TaskRelationshipMgr::DelLocalFlushJobId(const std::string &jobId)
{
    localFlushJobIds_.erase(jobId);
}

int TaskRelationshipMgr::GetFlushSuffixDevId(const std::string &jobId, int indexId)
{
    if (localFlushJobIds_.find(jobId) != localFlushJobIds_.end()) {
        return indexId;
    } else {
        return GetHostIdByDevId(indexId);
    }
}
}  // namespace TaskHandle
}  // namespace Dvvp
}  // namespace Analysis
