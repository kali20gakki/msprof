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

#ifndef MSPTI_PARSER_TASK_CALCULATOR_H
#define MSPTI_PARSER_TASK_CALCULATOR_H

#include <vector>
#include <functional>
#include <map>
#include <list>
#include <memory>
#include <typeindex>
#include <mutex>
#include <unordered_map>
#include "csrc/activity/ascend/channel/channel_data.h"
#include "csrc/common/utils.h"
#include "csrc/include/mspti_result.h"
#include "csrc/activity/ascend/entity/soclog.h"

namespace Mspti {
namespace Parser {
struct SubTask {
    uint64_t start;
    uint64_t end;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t subTaskId;
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
    DeviceTask() = default;
    DeviceTask(uint64_t start, uint64_t end, uint32_t streamId, uint32_t taskId, uint32_t deviceId, bool isFfts = false,
        bool agingFlag = true)
        : start(start),
          end(end),
          streamId(streamId),
          taskId(taskId),
          deviceId(deviceId),
          isFfts(isFfts),
          agingFlag(agingFlag)
    {}
};

class DeviceTaskCalculator {
    // <deviceId, streamId, taskId>
    using DstType = std::tuple<uint16_t, uint16_t, uint32_t>;

    // <deviceId, streamId, taskId, subTaskId>
    using DstsType = std::tuple<uint16_t, uint16_t, uint32_t, uint16_t>;

public:
    using CompleteFunc = std::function<msptiResult(DeviceTask&)>;

    static DeviceTaskCalculator &GetInstance()
    {
        static DeviceTaskCalculator instance;
        return instance;
    }

    void RegisterCallBack(const DeviceTask &assembleTask,
                          const DeviceTaskCalculator::CompleteFunc& completeFunc);
    msptiResult ReportStarsSocLog(uint32_t deviceId, const HalLogData& originData);

private:
    DeviceTaskCalculator() = default;

    msptiResult AssembleTasksTimeWithSocLog(uint32_t deviceId, const SocLog& socLog);

    msptiResult AssembleSubTasksTimeWithFftsLog(uint32_t deviceId, const FftsLog& fftsLog);

    void DealCacheDeviceTask();

private:
    std::unordered_map<DstType, DeviceTask, Common::TupleHash> assembleTasks_;
    std::unordered_map<DstsType, SubTask, Common::TupleHash> assembleSubTasks_;
    std::unordered_map<DstType, CompleteFunc, Common::TupleHash> completeFunc_;

    std::mutex assembleTaskMutex_;
    std::vector<std::pair<DeviceTask, CompleteFunc>> assembleTaskQueue_{};
    std::vector<std::pair<DeviceTask, CompleteFunc>> dealAssembleTaskQueue_{};
};
}
}

#endif // MSPTI_PARSER_TASK_CALCULATOR_H
