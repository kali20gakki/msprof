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

#include <algorithm>
#include "utils.h"

namespace Analysis {
namespace Parser {
namespace Adapter {
namespace {
const uint16_t STREAM_DESTROY_FLIP = 65535;
const uint32_t TASK_ID_BIT = 0x0000FFFF;
const uint16_t TASK_ID_BIT_NUM = 16;
}

uint16_t Flip::GetTaskId(const std::shared_ptr<MsprofCompactInfo> &task)
{
    return task->data.runtimeTrack.taskId & TASK_ID_BIT;  // taskId是低16位
}

uint16_t Flip::GetBatchId(const std::shared_ptr<MsprofCompactInfo> &task)
{
    return task->data.runtimeTrack.taskId >> TASK_ID_BIT_NUM;  // batchId/filpNum是高16位
}

void Flip::SetBatchId(std::shared_ptr<MsprofCompactInfo> &task, uint32_t batchId)
{
    task->data.runtimeTrack.taskId = (task->data.runtimeTrack.taskId & TASK_ID_BIT) + (batchId << TASK_ID_BIT_NUM);
}

std::shared_ptr<FlipTask> Flip::CreateFlipTask(const std::shared_ptr<MsprofCompactInfo> &taskTrack)
{
    std::shared_ptr<FlipTask> flipTask = Utils::MakeShared<FlipTask>();
    if (!flipTask || !taskTrack) {
        return nullptr;
    }
    flipTask->streamId = taskTrack->data.runtimeTrack.streamId;
    flipTask->taskId = GetTaskId(taskTrack);
    flipTask->timeStamp = taskTrack->timeStamp;
    flipTask->flipNum = GetBatchId(taskTrack);
    return flipTask;
}

void Flip::ComputeBatchId(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                          std::vector<std::shared_ptr<FlipTask>> &flipTask)
{
    if (taskTrack.empty()) {
        return;
    }
    std::sort(taskTrack.begin(), taskTrack.end(),
              [&](const std::shared_ptr<MsprofCompactInfo> &l, const std::shared_ptr<MsprofCompactInfo> &r) {
                  return l->timeStamp < r->timeStamp;
              });
    std::sort(flipTask.begin(), flipTask.end(),
              [&](const std::shared_ptr<FlipTask> &l, const std::shared_ptr<FlipTask> &r) {
                  return l->timeStamp < r->timeStamp;
              });
    auto taskTrackBin = SepTaskTrack(taskTrack);
    auto flipTaskBin = SepFlipTask(flipTask);
    for (auto &item : taskTrackBin) {
        auto streamId = item.first;
        std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrackInStream = item.second;
        std::vector<std::shared_ptr<FlipTask>> flipTaskInStream;
        auto iter = flipTaskBin.find(streamId);
        if (iter != flipTaskBin.end()) {
            flipTaskInStream = iter->second;
        }
        std::shared_ptr<FlipTask> maxFlip = Utils::MakeShared<FlipTask>();
        if (!maxFlip) {
            ERROR("Max flip make_shared failed");
            return;
        }
        flipTaskInStream.emplace_back(maxFlip);
        uint32_t batchId = 0;
        uint32_t taskIdx = 0;
        while (taskIdx < taskTrackInStream.size()) {
            auto &task = taskTrackInStream[taskIdx];
            auto flip = flipTaskInStream[batchId];
            if (task->timeStamp > flip->timeStamp) {
                ++batchId;  // next flip
                CalibrateFlipTaskIdNotZero(taskTrackInStream, flip, taskIdx, batchId);
                continue;
            }
            SetBatchId(task, batchId);
            ++taskIdx; // next task
        }
    }
}

Flip::CompactInfoMap Flip::SepTaskTrack(const std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack)
{
    Flip::CompactInfoMap data;
    for (const auto &task : taskTrack) {
        data[task->data.runtimeTrack.streamId].emplace_back(task);
    }
    return data;
}

Flip::FlipTaskMap Flip::SepFlipTask(const std::vector<std::shared_ptr<FlipTask>> &flipTask)
{
    Flip::FlipTaskMap data;
    for (const auto &flip : flipTask) {
        data[flip->streamId].emplace_back(flip);
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
    uint32_t taskIdxBackward = taskIdx - 1;
    while (taskIdxBackward >= 0 && GetTaskId(taskTrack[taskIdxBackward]) < flip->taskId) {
        SetBatchId(taskTrack[taskIdxBackward], batchId);
        --taskIdxBackward;
    }
}
}  // namespace Adapter
}  // namespace Parser
}  // namespace Analysis
