/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_task_calculator.cpp
 * Description        : 解析上报device上task耗时(大算子)
 * Author             : msprof team
 * Creation Date      : 2025/5/18
 * *****************************************************************************
 */

#include "device_task_calculator.h"

#include "stars_common.h"
#include "common/context_manager.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "activity/activity_manager.h"

namespace Mspti {
namespace Parser {
void DeviceTaskCalculator::RegisterCallBack(const std::vector<std::shared_ptr<DeviceTask>> &assembleTasks,
    DeviceTaskCalculator::CompleteFunc completeFunc)
{
    std::lock_guard<std::mutex> lockGuard(assembleTaskMutex_);
    for (auto &task : assembleTasks) {
        if (!task) {
            continue;
        }
        DstType dstKey = std::make_tuple(task->deviceId, task->streamId, task->taskId);
        assembleTasks_[dstKey].emplace_back(task);
        completeFunc_[dstKey].push_back(completeFunc);
    }
}

msptiResult DeviceTaskCalculator::ReportStarsSocLog(uint32_t deviceId, StarsSocHeader *socLogHeader)
{
    if (!socLogHeader) {
        return MSPTI_ERROR_INNER;
    }

    if (socLogHeader->funcType == STARS_FUNC_TYPE_BEGIN || socLogHeader->funcType == STARS_FUNC_TYPE_END) {
        return AssembleTasksTimeWithSocLog(deviceId, Common::ReinterpretConvert<StarsSocLog *>(socLogHeader));
    }

    if (socLogHeader->funcType == FFTS_PLUS_TYPE_END || socLogHeader->funcType == FFTS_PLUS_TYPE_START) {
        return AssembleSubTasksTimeWithFftsLog(deviceId, Common::ReinterpretConvert<FftsPlusLog *>(socLogHeader));
    }
    MSPTI_LOGW("stars log from device %u is not ffts or stars, funcType is %u", deviceId, socLogHeader->funcType);
    return MSPTI_SUCCESS;
}

msptiResult DeviceTaskCalculator::AssembleTasksTimeWithSocLog(uint32_t deviceId, StarsSocLog *socLog)
{
    uint16_t streamId = StarsCommon::GetStreamId(socLog->streamId, socLog->taskId);
    uint16_t taskId = StarsCommon::GetTaskId(socLog->streamId, socLog->taskId);
    auto dstKey = std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId);
    std::shared_ptr<Mspti::Parser::DeviceTask> deviceTask = nullptr;
    msptiResult ans = MSPTI_SUCCESS;
    {
        std::lock_guard<std::mutex> lk(assembleTaskMutex_);
        auto iter = assembleTasks_.find(dstKey);
        if (iter == assembleTasks_.end() || iter->second.empty()) {
            return MSPTI_SUCCESS;
        }
        deviceTask = iter->second.front();
        if (socLog->funcType == STARS_FUNC_TYPE_BEGIN) {
            deviceTask->start =
                Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId, socLog->timestamp);
        } else if (socLog->funcType == STARS_FUNC_TYPE_END) {
            deviceTask->end =
                Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId, socLog->timestamp);
            ans = completeFunc_[dstKey].front()(deviceTask);
            completeFunc_[dstKey].pop_front();
            assembleTasks_[dstKey].pop_front();
        }
    }

    {
        std::lock_guard<std::mutex> lk(assembleSubTaskMutex_);
        if (socLog->funcType == STARS_FUNC_TYPE_END && deviceTask) {
            for (auto &subTask : deviceTask->subTasks) {
                auto dstsKey =
                        std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId, subTask->subTaskId);
                assembleSubTasks_[dstsKey].pop_front();
            }
        }
    }
    return ans;
}

msptiResult DeviceTaskCalculator::AssembleSubTasksTimeWithFftsLog(uint32_t deviceId, FftsPlusLog *fftsLog)
{
    uint16_t streamId = StarsCommon::GetStreamId(fftsLog->streamId, fftsLog->taskId);
    uint16_t taskId = StarsCommon::GetTaskId(fftsLog->streamId, fftsLog->taskId);
    auto dstKey = std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId);
    auto dstsKey = std::make_tuple(static_cast<uint16_t>(deviceId), streamId, taskId, fftsLog->subTaskId);

    {
        std::lock_guard<std::mutex> lk(assembleTaskMutex_);
        auto iter = assembleTasks_.find(dstKey);
        if (iter == assembleTasks_.end() || iter->second.empty()) {
            return MSPTI_SUCCESS;
        }
    }

    {
        std::lock_guard<std::mutex> lk(assembleSubTaskMutex_);
        if (!assembleSubTasks_.count(dstsKey)) {
            std::shared_ptr<SubTask> subTask;
            Mspti::Common::MsptiMakeSharedPtr(subTask);
            if (!UNLIKELY(subTask)) {
                MSPTI_LOGE("fail to malloc subTask");
                return MSPTI_ERROR_INNER;
            }
            assembleSubTasks_[dstsKey].push_back(subTask);
        }
        auto subTask = assembleSubTasks_[dstsKey].front();
        if (fftsLog->funcType == FFTS_PLUS_TYPE_START) {
            subTask->start =
                Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId, fftsLog->timestamp);
            subTask->streamId = streamId;
            subTask->taskId = taskId;
            subTask->subTaskId = fftsLog->subTaskId;
        } else {
            subTask->end =
                Mspti::Common::ContextManager::GetInstance()->GetRealTimeFromSysCnt(deviceId, fftsLog->timestamp);
            {
                std::lock_guard<std::mutex> guard(assembleTaskMutex_);
                assembleTasks_[dstKey].front()->isFfts = true;
                assembleTasks_[dstKey].front()->subTasks.emplace_back(subTask);
            }
        }
    }
    return MSPTI_SUCCESS;
}
}
}
