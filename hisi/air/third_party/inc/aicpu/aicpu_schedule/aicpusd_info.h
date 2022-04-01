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

#ifndef AICPUSD_AICPUSD_INFO_H_
#define AICPUSD_AICPUSD_INFO_H_

#include <cstdint>
#include <sys/types.h>

extern "C" {

enum __attribute__((visibility("default"))) ProfilingMode {
    PROFILING_CLOSE = 0,
    PROFILING_OPEN,
};

struct __attribute__((visibility("default"))) AICPUSubEventStreamInfo {
    uint32_t streamId;
};

struct __attribute__((visibility("default"))) AICPUProfilingModeInfo {
    uint32_t deviceId;
    pid_t hostpId;
    uint32_t flag;
};

struct __attribute__((visibility("default"))) AICPULoadSoInfo {
    uint32_t kernelSoIndex;
};

struct __attribute__((visibility("default"))) AICPUEndGraphInfo {
    uint32_t result;
};

struct __attribute__((visibility("default"))) AICPUSubEventInfo {
    uint32_t modelId;
    union {
        AICPUSubEventStreamInfo streamInfo;
        AICPUProfilingModeInfo modelInfo;
        AICPULoadSoInfo loadSoInfo;
        AICPUEndGraphInfo endGraphInfo;
    } para;
};

struct ModelQueueInfo {
    uint32_t queueId;
    uint32_t flag;
} __attribute__((packed));

struct ModelTaskInfo {
    uint32_t taskId;
    uint64_t kernelName;
    uint64_t paraBase;
} __attribute__((packed));

struct ModelStreamInfo {
  uint32_t streamId;
  uint32_t streamFlag;
  uint16_t taskNum;
  ModelTaskInfo *tasks;
} __attribute__((packed));

struct ModelInfo {
    uint32_t modelId;
    uint16_t aicpuStreamNum;
    ModelStreamInfo *streams;
    uint16_t queueNum;
    ModelQueueInfo *queues;
    char rsv[128];
} __attribute__((packed));

struct CpuSchedInitParam {
    uint32_t deviceId;
    pid_t hostPid;
    ProfilingMode profilingMode;
    char rsv[128];
} __attribute__((packed));
}
#endif  // AICPUSD_AICPUSD_INFO_H_
