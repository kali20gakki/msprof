/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_task_calculator.cpp
 * Description        : 解析上报device上task耗时
 * Author             : msprof team
 * Creation Date      : 2025/5/18
 * *****************************************************************************
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
#include "activity/ascend/channel/channel_data.h"
#include "common/utils.h"
#include "external/mspti_result.h"

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
    bool isFfts;
    uint64_t start;
    uint64_t end;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t deviceId;
    std::vector<std::shared_ptr<SubTask>> subTasks;
    DeviceTask(uint64_t start, uint64_t end, uint32_t streamId, uint32_t taskId, uint32_t deviceId)
        : start(start), end(end), streamId(streamId), taskId(taskId), deviceId(deviceId), isFfts(false)
    {}
};

class DeviceTaskCalculator {
    // <deviceId, streamId, taskId>
    using DstType = std::tuple<uint16_t, uint16_t, uint16_t>;

    // <deviceId, streamId, taskId, subTaskId>
    using DstsType = std::tuple<uint16_t, uint16_t, uint16_t, uint16_t>;

public:
    using CompleteFunc = std::function<msptiResult(std::shared_ptr<DeviceTask>)>;

    static DeviceTaskCalculator &GetInstance()
    {
        static DeviceTaskCalculator instance;
        return instance;
    }

    void RegisterCallBack(const std::vector<std::shared_ptr<DeviceTask>> &assembleTasks, CompleteFunc completeFunc);

    msptiResult ReportStarsSocLog(uint32_t deviceId, StarsSocHeader *socLogHeader);

private:
    DeviceTaskCalculator() = default;

    msptiResult AssembleTasksTimeWithSocLog(uint32_t deviceId, StarsSocLog *socLog);

    msptiResult AssembleSubTasksTimeWithFftsLog(uint32_t deviceId, FftsPlusLog *fftsLog);

private:
    std::unordered_map<DstType, std::list<std::shared_ptr<DeviceTask>>, Common::TupleHash> assembleTasks_;
    std::unordered_map<DstsType, std::list<std::shared_ptr<SubTask>>, Common::TupleHash> assembleSubTasks_;
    std::unordered_map<DstType, std::list<CompleteFunc>, Common::TupleHash> completeFunc_;
    std::mutex assembleTaskMutex_;
    std::mutex assembleSubTaskMutex_;
};
}
}

#endif // MSPTI_PARSER_TASK_CALCULATOR_H
