/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : flip.cpp
 * Description        : batchId计算
 * Author             : msprof team
 * Creation Date      : 2023/11/18
 * *****************************************************************************
 */

#include "flip.h"

#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis {
namespace Domain {
namespace Adapter {
using namespace Analysis::Utils;

namespace {
const uint16_t STREAM_DESTROY_FLIP = 65535;
const uint32_t TASK_ID_BIT = 0x0000FFFF;
const uint16_t BIT_NUM = 16;
}  // namespace

uint16_t Flip::GetTaskId(const MsprofCompactInfo &task)
{
    return task.data.runtimeTrack.taskId & TASK_ID_BIT;  // taskId是低16位
}

uint16_t Flip::GetBatchId(const MsprofCompactInfo &task)
{
    return task.data.runtimeTrack.taskId >> BIT_NUM;  // batchId/flipNum是高16位
}

void Flip::SetBatchId(MsprofCompactInfo &task, uint32_t batchId)
{
    task.data.runtimeTrack.taskId = (task.data.runtimeTrack.taskId & TASK_ID_BIT) + (batchId << BIT_NUM);
}

std::shared_ptr<FlipTask> Flip::CreateFlipTask(MsprofCompactInfo *taskTrack)
{
    if (!taskTrack) {
        ERROR("Task track is null.");
        return nullptr;
    }
    std::shared_ptr<FlipTask> flipTask;
    MAKE_SHARED0_RETURN_VALUE(flipTask, FlipTask, nullptr);

    flipTask->deviceId = taskTrack->data.runtimeTrack.deviceId;
    flipTask->streamId = taskTrack->data.runtimeTrack.streamId;
    flipTask->taskId = GetTaskId(*taskTrack);
    flipTask->timeStamp = taskTrack->timeStamp;
    flipTask->flipNum = GetBatchId(*taskTrack);
    return flipTask;
}

void Flip::ComputeBatchId(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                          std::vector<std::shared_ptr<FlipTask>> &flipTask)
{
    if (taskTrack.empty()) {
        WARN("Task tracks is empty.");
        return;
    }
    auto taskTrackBin = SepTaskTrack(taskTrack);
    auto flipTaskBin = SepFlipTask(flipTask);
    std::shared_ptr<FlipTask> maxFlip;
    MAKE_SHARED0_RETURN_VOID(maxFlip, FlipTask);

    maxFlip->timeStamp = std::numeric_limits<uint64_t>::max();
    for (auto &item : taskTrackBin) {
        const auto &key = item.first;
        auto &taskTrackInStream = item.second;
        std::vector<std::shared_ptr<FlipTask>> flipTaskInStream;
        auto iter = flipTaskBin.find(key);
        if (iter != flipTaskBin.end()) {
            flipTaskInStream = iter->second;
        }
        flipTaskInStream.emplace_back(maxFlip);
        if (flipTaskInStream.size() > UINT16_MAX) {
            ERROR("The size of flip task in device_id % and stream_id % is greater than %.",
                  flipTaskInStream[0]->deviceId, flipTaskInStream[0]->streamId, UINT16_MAX);
            continue;
        }
        uint32_t batchId = 0;
        uint32_t taskIdx = 0;
        while (taskIdx < taskTrackInStream.size() && batchId < flipTaskInStream.size()) {
            auto &task = taskTrackInStream[taskIdx];
            const auto &flip = flipTaskInStream[batchId];
            if (task->timeStamp > flip->timeStamp) {
                ++batchId;  // next flip
                CalibrateFlipTaskIdNotZero(taskTrackInStream, flip, taskIdx, batchId);
                continue;
            }
            SetBatchId(*task, batchId);
            ++taskIdx; // next task
        }
    }
}

Flip::CompactInfoMap Flip::SepTaskTrack(const std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack)
{
    CompactInfoMap data;
    for (const auto &task : taskTrack) {
        if (!task) {
            ERROR("Task track is null.");
            continue;
        }
        uint32_t key = (task->data.runtimeTrack.deviceId << BIT_NUM) + task->data.runtimeTrack.streamId;
        data[key].emplace_back(task);
    }
    return data;
}

Flip::FlipTaskMap Flip::SepFlipTask(const std::vector<std::shared_ptr<FlipTask>> &flipTask)
{
    FlipTaskMap data;
    for (const auto &flip : flipTask) {
        if (!flip) {
            ERROR("Flip task is null.");
            continue;
        }
        uint32_t key = (flip->deviceId << BIT_NUM) + flip->streamId;
        data[key].emplace_back(flip);
    }
    return data;
}

void Flip::CalibrateFlipTaskIdNotZero(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                                      const std::shared_ptr<FlipTask> &flip, uint32_t taskIdx, uint32_t batchId)
{
    if (flip->flipNum == STREAM_DESTROY_FLIP) {
        return;
    }
    // Because tasks in multi-threads will apply for task id 0 simultaneously,
    // the flip may not get the task_id 0, we should search backward to calibrate the task
    // which task id is less than flip's task_id, and set these tasks the next batch id.
    while (taskIdx >= 1 && GetTaskId(*taskTrack[taskIdx - 1]) < flip->taskId) {
        SetBatchId(*taskTrack[taskIdx - 1], batchId);
        --taskIdx;
    }
}
}  // namespace Adapter
}  // namespace Parser
}  // namespace Analysis
