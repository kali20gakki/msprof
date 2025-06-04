/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_calculator.cpp
 * Description        : 解析上报记录通信数据
 * Author             : msprof team
 * Creation Date      : 2025/5/18
 * *****************************************************************************
 */

#include "communication_calculator.h"
#include "cann_hash_cache.h"

#include <algorithm>

#include "device_task_calculator.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "activity/activity_manager.h"
#include "common/context_manager.h"

namespace Mspti {
namespace Parser {
CommunicationCalculator &CommunicationCalculator::GetInstance()
{
    static CommunicationCalculator instance;
    return instance;
}

msptiResult CommunicationCalculator::AppendCompactInfo(const MsprofCompactInfo *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto &msprofHcclOpInfo = data->data.hcclopInfo;
    std::shared_ptr<msptiActivityCommunication> communication;
    Mspti::Common::MsptiMakeSharedPtr(communication);
    if (!UNLIKELY(communication)) {
        MSPTI_LOGE("fail to malloc msptiActivityCommunication, return MSPTI_ERROR_INNER");
        return MSPTI_ERROR_INNER;
    }
    communication->dataType = static_cast<msptiCommunicationDataType>(data->data.hcclopInfo.dataType);
    communication->kind = MSPTI_ACTIVITY_KIND_COMMUNICATION;
    communication->correlationId = Mspti::Common::ContextManager::GetInstance()->GetCorrelationId(data->threadId);
    communication->commName = CannHashCache::GetInstance().GetHashInfo(msprofHcclOpInfo.groupName).c_str();
    communication->algType = CannHashCache::GetInstance().GetHashInfo(msprofHcclOpInfo.algType).c_str();
    communication->count = msprofHcclOpInfo.count;

    std::lock_guard<std::mutex> lk(communicationOpInfoMutex_);
    communicationOpInfoQueue_[data->threadId].push(communication);
    return MSPTI_SUCCESS;
}

std::shared_ptr<DeviceTask> TransCommTask2DeviceTask(const std::shared_ptr<CommunicationTask> commTask)
{
    std::shared_ptr<DeviceTask> ans;
    Mspti::Common::MsptiMakeSharedPtr(ans, 0, 0, commTask->streamId, commTask->taskId, commTask->deviceId);
    return ans;
}

std::shared_ptr<CommunicationTask> TransApiEvent2CommTask(const ApiEvent *api2TaskInfo)
{
    auto track = api2TaskInfo->compactInfo.data.runtimeTrack;
    auto taskId = static_cast<uint16_t>(track.taskInfo & 0xffff);

    std::shared_ptr<CommunicationTask> ans;
    Mspti::Common::MsptiMakeSharedPtr(ans, 0, 0, api2TaskInfo->api.beginTime, api2TaskInfo->api.endTime,
                                      track.streamId, taskId, track.deviceId);
    return ans;
}

std::shared_ptr<CommunicationOpDesc> TransApiEvent2CommOpDesc(const ApiEvent *api2TaskInfo)
{
    std::shared_ptr<CommunicationOpDesc> desc;
    Mspti::Common::MsptiMakeSharedPtr(desc);
    if (!UNLIKELY(desc)) {
        return desc;
    }
    desc->opNameHash = api2TaskInfo->api.itemId;
    desc->hostStartTime = api2TaskInfo->api.beginTime;
    desc->hostEndTime = api2TaskInfo->api.endTime;
    desc->threadId = api2TaskInfo->api.threadId;
    desc->opNameHash = api2TaskInfo->api.itemId;
    for (auto &communicationTask : api2TaskInfo->childs) {
        if (communicationTask->level == MSPROF_REPORT_HCCL_NODE_LEVEL) {
            desc->tasks.emplace_back(TransApiEvent2CommTask(communicationTask.get()));
        }
    }
    return desc;
}

msptiResult CommunicationCalculator::AppendApi2TaskInfo(const std::shared_ptr<ApiEvent> api2TaskInfo)
{
    if (api2TaskInfo->childs.empty()) {
        MSPTI_LOGW("target Api has not communication tasks");
        return MSPTI_SUCCESS;
    }

    auto commOp = TransApiEvent2CommOpDesc(api2TaskInfo.get());
    if (commOp->tasks.empty()) {
        MSPTI_LOGW("commOp has not communication tasks");
    }
    std::sort(commOp->tasks.begin(), commOp->tasks.end(),
        [](const std::shared_ptr<CommunicationTask> a, const std::shared_ptr<CommunicationTask> b) {
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
        [this](std::shared_ptr<DeviceTask> task) { return Record(task); });
    return MSPTI_SUCCESS;
}

void AssembleTaskInfo(CommunicationOpDesc *opDesc, msptiActivityCommunication *communication)
{
    communication->ds.deviceId = opDesc->deviceId;
    communication->ds.streamId = opDesc->streamId;
    communication->start = opDesc->startTime;
    communication->end = opDesc->endTime;
    communication->name = CannHashCache::GetInstance().GetHashInfo(opDesc->opNameHash).c_str();
}

msptiResult CommunicationCalculator::ReportCommunication(std::shared_ptr<CommunicationOpDesc> commOp)
{
    std::shared_ptr<msptiActivityCommunication> communication;

    {
        std::lock_guard<std::mutex> lk(communicationOpInfoMutex_);
        communication = communicationOpInfoQueue_[commOp->threadId].front();
        communicationOpInfoQueue_[commOp->threadId].pop();
    }

    AssembleTaskInfo(commOp.get(), communication.get());
    Mspti::Activity::ActivityManager::GetInstance()->Record(
        Common::ReinterpretConvert<msptiActivity *>(communication.get()), sizeof(msptiActivityCommunication));
    return MSPTI_SUCCESS;
}

msptiResult CommunicationCalculator::Record(std::shared_ptr<DeviceTask> taskTime)
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
        commOp->streamId = taskTime->streamId;
        commOp->deviceId = taskTime->deviceId;
        if (taskTime->isFfts && !taskTime->subTasks.empty()) {
            std::sort(taskTime->subTasks.begin(), taskTime->subTasks.end(),
                [](const std::shared_ptr<SubTask> a, const std::shared_ptr<SubTask> b) {
                    return a->start < b->start;
                });
            commOp->startTime = taskTime->subTasks.front()->start;
        } else {
            commOp->startTime = taskTime->start;
        }
        firstTask2CommunicationOp_.erase(iter);
    }

    iter = lastTask2CommunicationOp_.find(dstType);
    if (iter != lastTask2CommunicationOp_.end()) {
        auto commOp = iter->second;
        if (taskTime->isFfts && !taskTime->subTasks.empty()) {
            std::sort(taskTime->subTasks.begin(), taskTime->subTasks.end(),
                [](const std::shared_ptr<SubTask> a, const std::shared_ptr<SubTask> b) {
                    return a->end > b->end;
                });
            commOp->endTime = taskTime->subTasks.front()->end;
        } else {
            commOp->endTime = taskTime->end;
        }
        ReportCommunication(commOp);
        lastTask2CommunicationOp_.erase(iter);
    }
    return MSPTI_SUCCESS;
}
}
}