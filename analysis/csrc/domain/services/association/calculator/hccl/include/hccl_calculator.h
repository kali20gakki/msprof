/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_calculator.h
 * Description        : HCCL数据计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_HCCL_CALCULATOR_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_HCCL_CALCULATOR_H

#include <limits>
#include <vector>

#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/domain/entities/hccl/include/hccl_task.h"
#include "analysis/csrc/infrastructure/process/include/process.h"
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis {
namespace Domain {
using namespace Infra;

struct DeviceHcclTask {
    std::string notifyId;
    std::string rdmaType;
    uint16_t iteration = 0;
    uint16_t batchId = 0;
    uint16_t taskId = 0;
    uint16_t isMaster = 0;
    int32_t indexId = 0;
    int32_t planeId = 0;
    uint32_t iterationId = 0;
    uint32_t streamId = 0;
    uint32_t localRank = 0;
    uint32_t remoteRank = 0;
    uint32_t contextId = 0;
    uint32_t threadId = 0;
    int64_t connectionId = 0;
    double timestamp = 0;
    double duration = 0;
    uint64_t hostTimestamp = 0;
    uint64_t firstTimestamp = 0;
    uint64_t modelId = 0;
    double durationEstimated = 0.0;
    double size = 0.0;
    double bandwidth = 0.0;
    std::string dataType;
    std::string linkType;
    std::string transportType;
    std::string isDynamic;
    std::string taskType;
    std::string opType;
    std::string hcclName;
    std::string groupName;
    std::string opName;
};

struct HcclStatistics {
    std::string opType;
    uint32_t count = 0;
    double totalTime = 0;
    double min = std::numeric_limits<double>::infinity();
    double max = 0;
    double avg = 0.0;
    double ratio = 0.0;
};

class HcclCalculator : public Process {
private:
    uint32_t ProcessEntry(DataInventory& dataInventory, const Context& context) override;
    bool GetHcclData(DataInventory& dataInventory);
    bool MergeHcclTaskData(const std::shared_ptr<std::vector<TopDownTask>>& ascendTasks,
                           const std::shared_ptr<std::vector<HcclTask>>& hcclTasks,
                           std::vector<DeviceHcclTask>& deviceHcclTasks);
    DeviceHcclTask InitHcclTaskData(const TopDownTask& topDownTask, const HcclTask& hcclTask);
    void MergeOpDataByThreadId(std::vector<HcclOp>& hcclOps, std::vector<DeviceHcclTask>& hcclTasks,
                               std::map<TaskId, uint16_t>& opCount);
    bool MergeHcclOpData(const std::shared_ptr<std::vector<HcclOp>>& hcclOps,
                         const std::vector<DeviceHcclTask>& deviceHcclTasks);
    DeviceHcclTask GetCompleteHcclTaskData(const HcclOp &op, const DeviceHcclTask &hcclTask, uint16_t count);
    HcclOp GetCompleteHcclOpData(const HcclOp &op);
    void UpdateHcclOpNameByGroupName();
    void UpdateHcclBandwidth();
    void CalculateTaskBandwidth(std::vector<DeviceHcclTask*> hcclTasks);
    uint16_t GetJumpNum(const DeviceHcclTask &task);
    double CalculateBandwidth(double size, double duration);
    uint16_t FindConsecutivePayloadTask(std::vector<DeviceHcclTask*> tasks, size_t idx);
    bool GetHcclStatisticsData(const Context& context);
    bool InjectData(DataInventory &inventory);
private:
    std::vector<HcclOp> opData_;
    std::vector<DeviceHcclTask> taskData_;
    std::vector<HcclStatistics> statisticsData_;
    bool isValidData_ = false;
};
}
}

#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_CALCULATOR_HCCL_CALCULATOR_H
