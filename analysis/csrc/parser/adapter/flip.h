/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : flip.h
 * Description        : batchId计算
 * Author             : msprof team
 * Creation Date      : 2023/11/18
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_ADAPTER_FLIP_H
#define ANALYSIS_PARSER_ADAPTER_FLIP_H

#include <vector>
#include <unordered_map>
#include <memory>

#include "collector/inc/toolchain/prof_common.h"

namespace Analysis {
namespace Parser {
namespace Adapter {
struct FlipTask {
    uint16_t deviceId;
    uint16_t streamId;
    uint16_t taskId;
    uint16_t flipNum;
    uint64_t timeStamp;
};

// 该类作用是根据FlipTask计算每个task的batch id
class Flip {
public:
    static void ComputeBatchId(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                               std::vector<std::shared_ptr<FlipTask>> &flipTask);
    static uint16_t GetTaskId(const MsprofCompactInfo &task);
    static uint16_t GetBatchId(const MsprofCompactInfo &task);
    static std::shared_ptr<FlipTask> CreateFlipTask(MsprofCompactInfo *taskTrack);

private:
    using CompactInfoMap = std::unordered_map<uint32_t, std::vector<std::shared_ptr<MsprofCompactInfo>>>;
    using FlipTaskMap = std::unordered_map<uint32_t, std::vector<std::shared_ptr<FlipTask>>>;
    static void CalibrateFlipTaskIdNotZero(std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack,
                                           const std::shared_ptr<FlipTask> &flip, uint32_t taskIdx, uint32_t batchId);
    static CompactInfoMap SepTaskTrack(const std::vector<std::shared_ptr<MsprofCompactInfo>> &taskTrack);
    static FlipTaskMap SepFlipTask(const std::vector<std::shared_ptr<FlipTask>> &flipTask);
    static void SetBatchId(MsprofCompactInfo &task, uint32_t batchId);
};  // class Flip
}  // namespace Adapter
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_ADAPTER_FLIP_H
