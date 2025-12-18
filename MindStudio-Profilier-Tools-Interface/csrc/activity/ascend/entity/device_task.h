/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_task.h
 * Description        : device侧的task耗时信息
 * Author             : msprof team
 * Creation Date      : 2025/11/11
 * *****************************************************************************
 */
#ifndef MSPTI_PROJECT_DEVICE_TASK_H
#define MSPTI_PROJECT_DEVICE_TASK_H

#include <cstdint>
#include <vector>
#include <memory>

struct SubTask {
    uint64_t start;
    uint64_t end;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t subTaskId;
};

struct HostTask {
    uint32_t threadId;
    uint16_t deviceId;
    uint16_t streamId;
    uint64_t taskType;
    uint64_t kernelHash;
    uint64_t corrleationId;
    uint32_t taskInfo;
    bool agingFlag;
    HostTask() = default;
    HostTask(uint32_t tid, uint16_t did, uint16_t sid, uint64_t taskType, uint64_t hash, uint64_t corrleationId,
             uint32_t taskInfo, bool agingFlag)
        : threadId(tid),
          deviceId(did),
          streamId(sid),
          taskType(taskType),
          kernelHash(hash),
          corrleationId(corrleationId),
          taskInfo(taskInfo),
          agingFlag(agingFlag) {}
};

struct DeviceTask {
    uint64_t start;
    uint64_t end;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t deviceId;
    bool isFfts;
    bool agingFlag;
    std::vector<SubTask> subTasks;
    DeviceTask()
        : start(0),
          end(0),
          streamId(0),
          taskId(0),
          deviceId(0),
          isFfts(false),
          agingFlag(true),
          subTasks(0)
    {
        subTasks.reserve(10);
    };
    DeviceTask(uint64_t start, uint64_t end, uint32_t streamId, uint32_t taskId, uint32_t deviceId, bool isFfts = false,
               bool agingFlag = true)
        : start(start),
          end(end),
          streamId(streamId),
          taskId(taskId),
          deviceId(deviceId),
          isFfts(isFfts),
          agingFlag(agingFlag),
          subTasks(0) {
        subTasks.reserve(10);
    }
};

#endif // MSPTI_PROJECT_DEVICE_TASK_H