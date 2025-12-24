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

#include "csrc/activity/ascend/parser/device_task_calculator.h"

#include "csrc/activity/ascend/channel/stars_common.h"
#include "csrc/common/context_manager.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/utils.h"
#include "csrc/activity/activity_manager.h"

namespace Mspti {
namespace Parser {
void DeviceTaskCalculator::RegisterCallBack(const DeviceTask &task,
                                            const DeviceTaskCalculator::CompleteFunc &completeFunc)
{
    DstType dstKey = std::make_tuple(task.deviceId, task.streamId, task.taskId);
    std::lock_guard<std::mutex> lockGuard(assembleTaskMutex_);
    assembleTaskQueue_.emplace_back(task, completeFunc);
        }

msptiResult DeviceTaskCalculator::ReportStarsSocLog(uint32_t deviceId, const HalLogData& originData)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    DealCacheDeviceTask();
    if (originData.type == ACSQ_LOG) {
        return AssembleTasksTimeWithSocLog(deviceId, originData.acsq);
    }

    if (originData.type == FFTS_LOG) {
        return AssembleSubTasksTimeWithFftsLog(deviceId, originData.ffts);
    }
    MSPTI_LOGW("stars log from device %u is not ffts or stars, funcType is %u", deviceId, originData.type);
    return MSPTI_SUCCESS;
}

msptiResult DeviceTaskCalculator::AssembleTasksTimeWithSocLog(uint32_t deviceId, const SocLog& socLog)
{
    uint16_t streamId = socLog.streamId;
    uint32_t taskId = socLog.taskId;
    auto dstKey = std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId);
        auto iter = assembleTasks_.find(dstKey);
    if (iter == assembleTasks_.end()) {
            return MSPTI_SUCCESS;
        }
    auto& deviceTask = iter->second;
    if (socLog.funcType == STARS_FUNC_TYPE_BEGIN) {
        deviceTask.start = socLog.timestamp;
    } else if (socLog.funcType == STARS_FUNC_TYPE_END) {
        std::vector<uint64_t> timeFromSysCnt = Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(
            deviceId, {deviceTask.start, socLog.timestamp});
        deviceTask.start = timeFromSysCnt[0];
        deviceTask.end = timeFromSysCnt[1];

        auto& callback = completeFunc_[dstKey];
        msptiResult ans = callback(deviceTask);
        if (deviceTask.agingFlag) {
                completeFunc_.erase(dstKey);
            assembleTasks_.erase(iter);
            }
        return ans;
                }
    return MSPTI_SUCCESS;
            }

msptiResult DeviceTaskCalculator::AssembleSubTasksTimeWithFftsLog(uint32_t deviceId, const FftsLog& fftsLog)
{
    uint16_t streamId = fftsLog.streamId;
    uint32_t taskId = fftsLog.taskId;
    auto dstKey = std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId);
    auto dstsKey = std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId, fftsLog.subTaskId);

        auto iter = assembleTasks_.find(dstKey);
    if (iter == assembleTasks_.end()) {
            return MSPTI_SUCCESS;
        }
    auto it = assembleSubTasks_.find(dstsKey);
    if (it == assembleSubTasks_.end()) {
        it = assembleSubTasks_.emplace(dstsKey, SubTask{}).first;
    }
    auto& subTask = it->second;
    if (fftsLog.funcType == FFTS_PLUS_TYPE_START) {
        subTask.start = fftsLog.timestamp;
        subTask.streamId = streamId;
        subTask.taskId = taskId;
        subTask.subTaskId = fftsLog.subTaskId;
    } else {
        auto timeFromSysCnt =
            Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId,
                                                                                {subTask.start, fftsLog.timestamp});
        subTask.start = timeFromSysCnt[0];
        subTask.end = timeFromSysCnt[1];
        iter->second.isFfts = true;
        iter->second.subTasks.emplace_back(subTask);
        assembleSubTasks_.erase(it);
            }
    return MSPTI_SUCCESS;
        }

void DeviceTaskCalculator::DealCacheDeviceTask()
            {
    {
        std::lock_guard<std::mutex> lk(assembleTaskMutex_);
        if (assembleTaskQueue_.empty()) {
            return;
            }
        dealAssembleTaskQueue_.reserve(assembleTaskQueue_.size());
        dealAssembleTaskQueue_.swap(assembleTaskQueue_);
            }
    for (const auto& it: dealAssembleTaskQueue_) {
        auto& deviceTask = it.first;
        auto& callbackFunc = it.second;
        DstType dstKey = std::make_tuple(deviceTask.deviceId, deviceTask.streamId, deviceTask.taskId);
        assembleTasks_.emplace(dstKey, deviceTask);
        completeFunc_.emplace(dstKey, callbackFunc);
        }
    dealAssembleTaskQueue_.clear();
    }
}
}
