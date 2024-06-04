/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_calculator.cpp
 * Description        : HCCL数据计算
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"

#include <algorithm>
#include <cctype>
#include <utility>
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {
namespace Domain {
namespace {
struct GroupData {
    uint64_t firstTimestamp = UINT64_MAX;
    uint16_t count = 0;
};

struct OpTypeInfo {
    OpTypeInfo() = default;
    OpTypeInfo(uint64_t max, uint64_t min, std::string opType) : max(max), min(min), opType(std::move(opType)) {}
    uint64_t max = 0;
    uint64_t min = UINT64_MAX;
    std::string opType;
};

const uint16_t RDMA_NO_BARRIER_TASK_NUM = 3;
const uint16_t RDMA_WITH_BARRIER_TASK_NUM = 5;
const uint16_t PERCENTAGE = 100;
const std::string RDMA_SEND_PAYLOAD = "RDMA_SEND_PAYLOAD";
const std::string NA = "N/A";
}

uint32_t HcclCalculator::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    INFO("Start Hccl calculator ProcessEntry.");
    if (!GetHcclData(dataInventory)) {
        ERROR("Failed to Get hccl task data or op data.");
        return ANALYSIS_ERROR;
    }
    UpdateHcclOpNameByGroupName();
    UpdateHcclBandwidth();
    if (!GetHcclStaticsData()) {
        ERROR("Failed to Get hccl stastics data.");
        return ANALYSIS_ERROR;
    }
    if (!InjectData(dataInventory)) {
        ERROR("Failed to inject hccl data.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

bool HcclCalculator::GetHcclData(DataInventory& dataInventory)
{
    INFO("Start Hccl calculator GetHcclData.");
    auto ascendTasks = dataInventory.GetPtr<std::vector<TopDownTask>>();
    auto hcclTasks = dataInventory.GetPtr<std::vector<HcclTask>>();
    std::vector<DeviceHcclTask> deviceHcclTasks;
    if (!MergeHcclTaskData(ascendTasks, hcclTasks, deviceHcclTasks)) {
        ERROR("Merge hccl task and ascend task failed.");
        return false;
    }

    auto hcclOps = dataInventory.GetPtr<std::vector<HcclOp>>();
    if (!MergeHcclOpData(hcclOps, deviceHcclTasks)) {
        ERROR("Merge hccl op failed.");
        return false;
    }
}

bool HcclCalculator::MergeHcclTaskData(const std::shared_ptr<std::vector<TopDownTask>>& ascendTasks,
                                       const std::shared_ptr<std::vector<HcclTask>>& hcclTasks,
                                       std::vector<DeviceHcclTask>& deviceHcclTasks)
{
    INFO("Start merge hccl task and ascend task.");
    std::sort(ascendTasks->begin(), ascendTasks->end(), [](const TopDownTask& task1, const TopDownTask& task2) {
        return task1.startTime < task2.startTime;
    });
    std::sort(hcclTasks->begin(), hcclTasks->end(), [](const HcclTask& task1, const HcclTask& task2) {
        return task1.timestamp < task2.timestamp;
    });
    std::map<TaskId, TopDownTask> taskTable;
    for (const auto& task : *ascendTasks) {
        TaskId tempId(task.streamId, task.batchId, task.taskId, task.contextId);
        taskTable[tempId] = task;
    }

    if (!Utils::Reserve(deviceHcclTasks, hcclTasks->size())) {
        ERROR("Reserve for hccl task failed.");
        return false;
    }
    for (const auto& task : *hcclTasks) {
        TaskId tempId(task.streamId, task.batchId, task.taskId, task.contextId);
        if (taskTable.find(tempId) == taskTable.end()) {
            ERROR("Hccl task can't match ascend task, streamId is: %, taskId is: %, contextId is: %, batchId is: %",
                  task.streamId, task.taskId, task.contextId, task.batchId);
            continue;
        }
        if (taskTable[tempId].startTime != UINT64_MAX) {
            deviceHcclTasks.emplace_back(InitHcclTaskData(taskTable[tempId], task));
        }
    }
}

DeviceHcclTask HcclCalculator::InitHcclTaskData(const TopDownTask &topDownTask, const HcclTask &hcclTask)
{
    DeviceHcclTask task;
    task.modelId = hcclTask.modelId;
    task.indexId = hcclTask.indexId;
    task.hcclName = hcclTask.name;
    task.planeId = hcclTask.planeId;
    task.hostTimestamp = hcclTask.timestamp;
    task.groupName = hcclTask.groupName;
    task.isMaster = hcclTask.isMaster;
    task.streamId = hcclTask.streamId;
    task.taskId = hcclTask.taskId;
    task.durationEstimated = hcclTask.duration;
    task.localRank = hcclTask.localRank;
    task.remoteRank = hcclTask.remoteRank;
    task.transportType = hcclTask.transportType;
    task.size = hcclTask.size;
    task.dataType = hcclTask.dataType;
    task.linkType = hcclTask.linkType;
    task.contextId = hcclTask.contextId;
    task.notifyId = hcclTask.notifyId;
    task.batchId = hcclTask.batchId;
    task.rdmaType = hcclTask.rdmaType;
    task.timestamp = topDownTask.startTime;
    task.connectionId = topDownTask.connectionId;
    task.duration = topDownTask.endTime - topDownTask.startTime;
    if (!isValidData_ && task.opType != NA) {
        isValidData_ = true;
    }
    return task;
}

bool HcclCalculator::MergeHcclOpData(const std::shared_ptr<std::vector<HcclOp>> &hcclOps,
                                     const std::vector<DeviceHcclTask>& deviceHcclTasks)
{
    INFO("Start merge hccl op data.");
    if (!Utils::Reserve(opData_, hcclOps->size())) {
        ERROR("Reserve for hccl op failed.");
        return false;
    }
    if (!Utils::Reserve(taskData_, deviceHcclTasks.size())) {
        ERROR("Reserve for hccl task failed.");
        return false;
    }
    std::sort(hcclOps->begin(), hcclOps->end(), [](const HcclOp& op1, const HcclOp& op2) {
        return op1.timestamp < op2.timestamp;
    });
    size_t taskIdx = 0;
    std::map<TaskId, uint16_t> opCount;
    for (const auto& op : *hcclOps) {
        while (taskIdx < deviceHcclTasks.size() && deviceHcclTasks[taskIdx].hostTimestamp < op.timestamp) {
            ERROR("Hccl task time not in ops time range, streamId is: %, taskId is: %, contextId is: %, batchId is: %",
                  deviceHcclTasks[taskIdx].streamId, deviceHcclTasks[taskIdx].taskId,
                  deviceHcclTasks[taskIdx].contextId, deviceHcclTasks[taskIdx].batchId);
            taskIdx++;
        }

        while ((taskIdx < deviceHcclTasks.size())
                && (deviceHcclTasks[taskIdx].hostTimestamp < (op.timestamp + op.duration))) {
            TaskId tempId(deviceHcclTasks[taskIdx].streamId, deviceHcclTasks[taskIdx].batchId,
                          deviceHcclTasks[taskIdx].taskId, deviceHcclTasks[taskIdx].contextId);
            uint16_t count = (opCount.find(tempId) == opCount.end()) ? 1 : (opCount[tempId] + 1);
            opCount[tempId] = count;
            taskData_.emplace_back(GetCompleteHcclTaskData(op, deviceHcclTasks[taskIdx], count));
            taskIdx++;
        }
        opData_.emplace_back(GetCompleteHcclOpData(op));
    }
    if (taskIdx < deviceHcclTasks.size() -1) {
        ERROR("Task_queue is not empty, len is: %", deviceHcclTasks.size());
    }
    if (taskData_.empty()) {
        ERROR("Communication data is empty.");
        return false;
    }
    return true;
}

DeviceHcclTask HcclCalculator::GetCompleteHcclTaskData(const HcclOp &op, const DeviceHcclTask &hcclTask, uint16_t count)
{
    DeviceHcclTask task = hcclTask;
    task.opName = op.opName;
    task.groupName = (hcclTask.groupName == NA) ? op.groupName : hcclTask.groupName;
    task.taskType = op.taskType;
    task.opType = op.opType;
    task.firstTimestamp = op.timestamp;
    task.iterationId = count;
    task.isDynamic = op.isDynamic;
    task.modelId = op.modelId;
    task.connectionId = op.connectionId;
    return task;
}

HcclOp HcclCalculator::GetCompleteHcclOpData(const HcclOp &op)
{
    HcclOp hcclOp;
    hcclOp.modelId = op.modelId;
    hcclOp.opName = op.opName;
    hcclOp.taskType = op.taskType;
    hcclOp.opType = op.opType;
    hcclOp.timestamp = op.timestamp;
    hcclOp.relay = op.relay;
    hcclOp.retry = op.retry;
    hcclOp.dataType = op.dataType;
    hcclOp.algType = op.algType;
    hcclOp.count = op.count;
    hcclOp.groupName = op.groupName;
    hcclOp.connectionId = op.connectionId;
    return hcclOp;
}

void HcclCalculator::UpdateHcclOpNameByGroupName()
{
    INFO("Start UpdateHcclOpNameByGroupName.");
    std::unordered_map<std::string, GroupData> hcclGroup;
    for (auto& data : taskData_) {
        if (hcclGroup.find(data.groupName) != hcclGroup.end()) {
            auto& group_entry = hcclGroup[data.groupName];
            if (data.firstTimestamp > group_entry.firstTimestamp) {
                group_entry.firstTimestamp = data.firstTimestamp;
                group_entry.count++;
            }
        } else {
            GroupData groupData;
            groupData.firstTimestamp = data.firstTimestamp;
            hcclGroup[data.groupName] = groupData;
        }
        auto subGroupName = data.groupName.substr((data.groupName.size() - 3 < 0) ? 0 : data.groupName.size() - 3);
        data.opName = Utils::Join("_", data.opName, subGroupName,
                                  std::to_string(hcclGroup[data.groupName].count), std::to_string(data.iterationId));
    }
}

void HcclCalculator::UpdateHcclBandwidth()
{
    INFO("Start UpdateHcclBandwidth.");
    // 按时间升序排序，确保后续payload遍历时数据顺序正确
    std::sort(taskData_.begin(), taskData_.end(), [](const DeviceHcclTask& task1, const DeviceHcclTask& task2) {
        return task1.timestamp < task2.timestamp;
    });
    std::unordered_map<std::string, std::unordered_map<int32_t, std::vector<DeviceHcclTask*>>> taskTable;
    for (auto& data : taskData_) {
        // 没有提前reserve，这里可能很耗时
        taskTable[data.opName][data.planeId].push_back(&data);
    }
    for (auto& planeTable : taskTable) {
        for (auto& taskData : planeTable.second) {
            CalculateTaskBandwidth(taskData.second);
        }
    }
}

void HcclCalculator::CalculateTaskBandwidth(std::vector<DeviceHcclTask*> hcclTasks)
{
    uint16_t idx_jump = GetJumpNum(*hcclTasks.front());
    for (size_t idx = 0; idx < hcclTasks.size(); ++idx) {
        // 非RDMA_SEND_PAYLOAD类型直接计算；RDMA_SEND_PAYLOAD类型走其他计算逻辑
        if (hcclTasks[idx]->rdmaType != RDMA_SEND_PAYLOAD) {
            hcclTasks[idx]->bandwidth = CalculateBandwidth(hcclTasks[idx]->size, hcclTasks[idx]->duration);
            continue;
        }
        uint16_t payloadCnt = FindConsecutivePayloadTask(hcclTasks, idx);
        auto closeIdx = idx + payloadCnt + idx_jump - 2;
        if ((closeIdx) >= hcclTasks.size()) {
            WARN("Bandwidth calculation abnormal. Missing closure tasks. op_name: %, index is: %, paypladCnt is: %, "
                 "idx_jump is: %,", hcclTasks[idx]->opName, idx, payloadCnt, idx_jump);
            hcclTasks[idx]->bandwidth = CalculateBandwidth(hcclTasks[idx]->size, hcclTasks[idx]->duration);\
            continue;
        }
        auto payLoadAllSize = hcclTasks[idx]->size;
        for (size_t sizeI = idx + 1; sizeI < idx + payloadCnt; ++sizeI) {
            payLoadAllSize += hcclTasks[sizeI]->size;
        }
        auto transitTime = hcclTasks[closeIdx]->timestamp + hcclTasks[closeIdx]->duration - hcclTasks[idx]->timestamp;
        double payloadBandwidth = (transitTime != 0) ? (payLoadAllSize / transitTime) : 0;
        for (size_t sizeI = idx; sizeI < idx + payloadCnt; ++sizeI) {
            hcclTasks[sizeI]->bandwidth = payloadBandwidth;
        }
        // 修改原有逻辑，下一个idx从连续的payload后开始。确保每个算子的bandwidth都被计算。
        idx += payloadCnt - 1;
    }
}

uint16_t HcclCalculator::GetJumpNum(const DeviceHcclTask &task)
{
    std::string opName = task.opName;
    transform(opName.begin(), opName.end(), opName.begin(), tolower);
    if (opName.find("send") != std::string::npos || opName.find("receive") != std::string::npos) {
        return RDMA_NO_BARRIER_TASK_NUM;
    }
    return RDMA_WITH_BARRIER_TASK_NUM;
}

double HcclCalculator::CalculateBandwidth(double size, uint64_t duration)
{
    // B -> GB: 以 1 / 10^9替代； ns -> s: 以 1 / 10^9替代。两者约分，带宽单位为 GB/s
    return (duration <= 0) ? 0 : static_cast<double>(size) / duration;
}

uint16_t HcclCalculator::FindConsecutivePayloadTask(std::vector<DeviceHcclTask*> tasks, size_t idx)
{
    uint16_t count = 0;
    while (idx < tasks.size() && tasks[idx]->rdmaType == RDMA_SEND_PAYLOAD) {
        idx++;
        count++;
    }
    return count;
}

bool HcclCalculator::GetHcclStaticsData()
{
    INFO("Start GetHcclStaticsData.");
    if (!isValidData_) {
        WARN("No op type in hccl data.");
        return true;
    }
    std::unordered_map<std::string, OpTypeInfo> groupedData;
    for (const auto& task : taskData_) {
        auto key = Utils::Join("-", task.opName, std::to_string(task.firstTimestamp), task.opType);
        if (groupedData.find(key) == groupedData.end()) {
            OpTypeInfo info(task.timestamp, task.timestamp, task.opType);
            groupedData[key] = info;
        } else {
            auto& temp = groupedData[key];
            temp.max = std::max(task.timestamp, temp.max);
            temp.min = std::min(task.timestamp, temp.min);
        }
    }
    std::unordered_map<std::string, HcclStastics> stasticsTable;
    uint64_t totalTotalTime = 0;
    for (const auto& data : groupedData) {
        auto& record =  stasticsTable[data.second.opType];
        record.opType = data.second.opType;
        record.count++;
        record.totalTime += data.second.max - data.second.min;
        record.max = std::max(data.second.max, record.max);
        record.min = std::min(data.second.min, record.min);
        totalTotalTime += data.second.max - data.second.min;
    }
    if (!Utils::Reserve(stasticsData_, stasticsTable.size())) {
        ERROR("Reserve for hccl stastics data failed.");
        return false;
    }
    for (auto& data : stasticsTable) {
        data.second.avg = static_cast<double>(data.second.totalTime) / data.second.count;
        data.second.ratio = static_cast<double>(data.second.totalTime) / totalTotalTime * PERCENTAGE;
        stasticsData_.emplace_back(data.second);
    }
    std::sort(stasticsData_.begin(), stasticsData_.end(), [](const HcclStastics& task1, const HcclStastics& task2) {
        return task1.ratio > task2.ratio;
    });
    return true;
}

bool HcclCalculator::InjectData(DataInventory &inventory)
{
    INFO("Start inject hccl data.");
    auto hcclOpData = inventory.GetPtr<std::vector<HcclOp>>();
    // 直接替换原有的HcclOp数据
    hcclOpData->swap(opData_);

    bool flag = true;
    std::shared_ptr<std::vector<DeviceHcclTask>> hcclTaskData;
    MAKE_SHARED0_NO_OPERATION(hcclTaskData, std::vector<DeviceHcclTask>, std::move(taskData_));
    if (!inventory.Inject(hcclTaskData)) {
        ERROR("Inject hccl task data failed.");
        flag = false;
    }

    std::shared_ptr<std::vector<HcclStastics>> hcclStasticsData;
    MAKE_SHARED0_NO_OPERATION(hcclStasticsData, std::vector<HcclStastics>, std::move(stasticsData_));
    if (!inventory.Inject(hcclStasticsData)) {
        ERROR("Inject hccl stastics data failed.");
        flag = false;
    }
    return flag;
}


REGISTER_PROCESS_SEQUENCE(HcclCalculator, true);
REGISTER_PROCESS_DEPENDENT_DATA(HcclCalculator, std::vector<TopDownTask>, std::vector<HcclTask>,
                                std::vector<HcclOp>);
REGISTER_PROCESS_SUPPORT_CHIP(HcclCalculator, CHIP_ID_ALL);
}
}