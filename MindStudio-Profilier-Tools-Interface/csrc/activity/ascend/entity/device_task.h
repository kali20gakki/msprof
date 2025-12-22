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