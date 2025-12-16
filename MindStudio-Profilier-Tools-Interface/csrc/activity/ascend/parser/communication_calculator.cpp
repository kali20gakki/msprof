/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#include "communication_calculator.h"
#include "cann_hash_cache.h"

#include <algorithm>

#include "device_task_calculator.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/utils.h"
#include "csrc/activity/activity_manager.h"
#include "csrc/common/context_manager.h"

namespace Mspti {
namespace Parser {
CommunicationCalculator &CommunicationCalculator::GetInstance()
{
    static CommunicationCalculator instance;
    return instance;
}

msptiResult CommunicationCalculator::AppendCompactInfo(bool agingFlag, const MsprofCompactInfo *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto &msprofHcclOpInfo = data->data.hcclopInfo;
    std::shared_ptr<CommunicationOpDesc> communication;
    Mspti::Common::MsptiMakeSharedPtr(communication);
    if (!UNLIKELY(communication)) {
        MSPTI_LOGE("fail to malloc msptiActivityCommunication, return MSPTI_ERROR_INNER");
        return MSPTI_ERROR_INNER;
    }
    communication->agingFlag = agingFlag;
    communication->dataType = data->data.hcclopInfo.dataType;
    communication->groupNameHash = msprofHcclOpInfo.groupName;
    communication->algTypeHash = msprofHcclOpInfo.algType;
    communication->count = msprofHcclOpInfo.count;
    std::lock_guard<std::mutex> lk(communicationOpInfoMutex_);
    communicationOpInfoQueue_[data->threadId].push(communication);
    return MSPTI_SUCCESS;
}

std::shared_ptr<DeviceTask> TransCommTask2DeviceTask(const std::unique_ptr<CommunicationTask>& commTask)
{
    std::shared_ptr<DeviceTask> ans;
    Mspti::Common::MsptiMakeSharedPtr(ans, 0, 0, commTask->streamId, commTask->taskId, commTask->deviceId, false,
                                      commTask->agingFlag);
    return ans;
}

std::unique_ptr<CommunicationTask> TransApiEvent2CommTask(const ApiEvent *api2TaskInfo)
{
    auto track = api2TaskInfo->compactInfo.data.runtimeTrack;
    auto taskId = static_cast<uint16_t>(track.taskInfo & 0xffff);
    std::unique_ptr<CommunicationTask> ans;
    Mspti::Common::MsptiMakeUniquePtr(ans, 0, 0, api2TaskInfo->api.beginTime, api2TaskInfo->api.endTime,
                                      track.streamId, taskId, track.deviceId, api2TaskInfo->agingFlag);
    return ans;
}

std::shared_ptr<CommunicationOpDesc> TransApiEvent2CommOpDesc(const ApiEvent *api2TaskInfo)
{
    std::shared_ptr<CommunicationOpDesc> desc;
    Mspti::Common::MsptiMakeSharedPtr(desc);
    if (!UNLIKELY(desc)) {
        return desc;
    }
    desc->correlationId = api2TaskInfo->correlationId;
    desc->opNameHash = api2TaskInfo->api.itemId;
    desc->hostStartTime = api2TaskInfo->api.beginTime;
    desc->hostEndTime = api2TaskInfo->api.endTime;
    desc->threadId = api2TaskInfo->api.threadId;
    desc->opNameHash = api2TaskInfo->api.itemId;
    for (auto &communicationTask : api2TaskInfo->children) {
        if (communicationTask->level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
            auto commTask = TransApiEvent2CommTask(communicationTask.get());
            if (UNLIKELY(commTask == nullptr)) {
                MSPTI_LOGE("can not TransApiEvent2CommTask, commTask is nullptr!");
                continue;
            }
            desc->agingFlag = commTask->agingFlag;
            desc->tasks.emplace_back(std::move(commTask));
        }
    }
    return desc;
}

msptiResult CommunicationCalculator::AppendApi2TaskInfo(const std::unique_ptr<ApiEvent>& api2TaskInfo)
{
    if (api2TaskInfo->children.empty()) {
        MSPTI_LOGW("target Api has not communication tasks");
        return MSPTI_SUCCESS;
    }

    auto commOp = TransApiEvent2CommOpDesc(api2TaskInfo.get());
    if (commOp->tasks.empty()) {
        MSPTI_LOGW("commOp has not communication tasks");
        return MSPTI_SUCCESS;
    }
    std::sort(commOp->tasks.begin(), commOp->tasks.end(),
        [](const std::unique_ptr<CommunicationTask>& a, const std::unique_ptr<CommunicationTask>& b) {
            return a->start < b->start; // 按 timestamp 升序排序
        });
    if (commOp->tasks.empty()) {
        MSPTI_LOGW("commOp has not communication tasks");
        return MSPTI_SUCCESS;
    }
    auto firstTask = TransCommTask2DeviceTask(commOp->tasks.front());
    auto lastTask = TransCommTask2DeviceTask(commOp->tasks.back());
    if (UNLIKELY(firstTask == nullptr || lastTask == nullptr)) {
        MSPTI_LOGE("firstTask or lastTask is nullptr!");
        return MSPTI_ERROR_INNER;
    }

    auto firstTaskDst = std::make_tuple(firstTask->deviceId, firstTask->streamId, firstTask->taskId);
    auto lastTaskDst = std::make_tuple(lastTask->deviceId, lastTask->streamId, lastTask->taskId);

    {
        std::lock_guard<std::mutex> lk(hcclTaskMutex_);
        firstTask2CommunicationOp_[firstTaskDst] = commOp;
        lastTask2CommunicationOp_[lastTaskDst] = commOp;
    }

    std::vector<std::shared_ptr<DeviceTask>> assembleTasks{ firstTask };
    if (firstTaskDst != lastTaskDst) {
        assembleTasks.push_back(lastTask);
    }
    DeviceTaskCalculator::GetInstance().RegisterCallBack(assembleTasks,
        [this](const std::shared_ptr<DeviceTask>& task) { return Record(task); });
    return MSPTI_SUCCESS;
}

void AssembleTaskInfo(msptiActivityCommunication& communication, const CommunicationOpDesc *additionOpDesc,
                      const CommunicationOpDesc *OpTaskDesc)
{
    communication.kind = MSPTI_ACTIVITY_KIND_COMMUNICATION;
    if (additionOpDesc != nullptr) {
        communication.dataType = static_cast<msptiCommunicationDataType>(additionOpDesc->dataType);
        communication.count = additionOpDesc->count;
        communication.algType = CannHashCache::GetInstance().GetHashInfo(additionOpDesc->algTypeHash).c_str();
        communication.commName = CannHashCache::GetInstance().GetHashInfo(additionOpDesc->groupNameHash).c_str();
    }

    if (OpTaskDesc != nullptr) {
        communication.ds.deviceId = OpTaskDesc->deviceId;
        communication.ds.streamId = OpTaskDesc->streamId;
        communication.start = OpTaskDesc->startTime;
        communication.end = OpTaskDesc->endTime;
        communication.name = CannHashCache::GetInstance().GetHashInfo(OpTaskDesc->opNameHash).c_str();
        communication.correlationId = OpTaskDesc->correlationId;
    }
}

msptiResult CommunicationCalculator::ReportCommunication(const DstType& dstKey,
                                                         const std::shared_ptr<CommunicationOpDesc>& commOp)
{
    std::shared_ptr<CommunicationOpDesc> additionOpDesc;
    {
        std::lock_guard<std::mutex> lk(communicationOpInfoMutex_);
        if (!commOp->agingFlag && taskId2AdditionInfo.count(dstKey)) {
            // 拿DstKey查一遍unordered_map，如果没有走正常逻辑
            additionOpDesc = taskId2AdditionInfo[dstKey];
        } else {
            auto& it =  communicationOpInfoQueue_[commOp->threadId];
            if (!it.empty()) {
                additionOpDesc = communicationOpInfoQueue_[commOp->threadId].front();
                communicationOpInfoQueue_[commOp->threadId].pop();
            }
            if (!commOp->agingFlag && additionOpDesc != nullptr) {
                taskId2AdditionInfo.emplace(dstKey, additionOpDesc);
            }
        }
    }
    msptiActivityCommunication record{};
    AssembleTaskInfo(record, additionOpDesc.get(), commOp.get());
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(&record), sizeof(msptiActivityCommunication));
    return MSPTI_SUCCESS;
}

msptiResult CommunicationCalculator::Record(const std::shared_ptr<DeviceTask>& taskTime)
{
    if (!taskTime) {
        MSPTI_LOGE("Record received null task");
        return MSPTI_ERROR_INNER;
    }
    auto dstType = std::make_tuple(taskTime->deviceId, taskTime->streamId, taskTime->taskId);
    std::lock_guard<std::mutex> lk(hcclTaskMutex_);
    auto iter = firstTask2CommunicationOp_.find(dstType);
    if (iter != firstTask2CommunicationOp_.end()) {
        auto commOp = iter->second;
        commOp->agingFlag = taskTime->agingFlag;
        commOp->streamId = taskTime->streamId;
        commOp->deviceId = taskTime->deviceId;
        if (taskTime->isFfts && !taskTime->subTasks.empty()) {
            std::sort(taskTime->subTasks.begin(), taskTime->subTasks.end(),
                [](const std::shared_ptr<SubTask>& a, const std::shared_ptr<SubTask>& b) {
                    return a->start < b->start;
                });
            commOp->startTime = taskTime->subTasks.front()->start;
        } else {
            commOp->startTime = taskTime->start;
        }
        if (commOp->agingFlag) {
            firstTask2CommunicationOp_.erase(iter);
        }
    }

    iter = lastTask2CommunicationOp_.find(dstType);
    if (iter != lastTask2CommunicationOp_.end()) {
        auto commOp = iter->second;
        if (taskTime->isFfts && !taskTime->subTasks.empty()) {
            std::sort(taskTime->subTasks.begin(), taskTime->subTasks.end(),
                [](const std::shared_ptr<SubTask>& a, const std::shared_ptr<SubTask>& b) {
                    return a->end > b->end;
                });
            commOp->endTime = taskTime->subTasks.front()->end;
        } else {
            commOp->endTime = taskTime->end;
        }
        ReportCommunication(dstType, commOp);
        if (commOp->agingFlag) {
            lastTask2CommunicationOp_.erase(iter);
        }
    }
    return MSPTI_SUCCESS;
}
}
}