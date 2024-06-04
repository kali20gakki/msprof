/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_task_association.cpp
 * Description        : Ascend_task关联
 * Author             : msprof team
 * Creation Date      : 2024/5/23
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/association/include/ascend_task_association.h"
#include <map>
#include <algorithm>
#include "analysis/csrc/domain/entities/hal/include/hal.h"
#include "analysis/csrc/domain/entities/hal/include/device_task.h"
#include "analysis/csrc/domain/entities/hal/include/top_down_task.h"
#include "analysis/csrc/entities/ascend_obj.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/domain/services/device_context/load_host_data.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Entities;
namespace {
const uint64_t DEFAULT_MODEL_ID = UINT64_MAX;
const int32_t DEFAULT_INDEX_ID = -1;
const int64_t DEFAULT_CONNECTION_ID = -1;
const std::unordered_map<uint32_t, std::string> deviceTaskAcsqTypeMap {
        {0, "AI_CORE"},
        {1, "AI_CPU"},
        {2, "AIV_SQE"},
        {3, "PLACE_HOLDER_SQE"},
        {4, "EVENT_RECORD_SQE"},
        {5, "EVENT_WAIT_SQE"},
        {6, "NOTIFY_RECORD_SQE"},
        {7, "NOTIFY_WAIT_SQE"},
        {8, "WRITE_VALUE_SQE"},
        {9, "VQ6_SQE"},
        {10, "TOF_SQE"},
        {11, "SDMA_SQE"},
        {12, "VPC_SQE"},
        {13, "JPEGE_SQE"},
        {14, "JPEGD_SQE"},
        {15, "DSA_SQE"},
        {16, "ROCCE_SQE"},
        {17, "PCIE_DMA_SQE"},
        {18, "HOST_CPU_SQE"},
        {19, "CDQM_SQE"},
        {20, "C_CORE_SQE"}
};

const std::unordered_map<uint32_t, std::string> deviceTaskFftsPlusTypeMap {
        {0, "AIC"},
        {1, "AIV"},
        {3, "Notify Wait"},
        {4, "Notify Record"},
        {5, "Write Value"},
        {6, "MIX_AIC"},
        {7, "MIX_AIV"},
        {8, "SDMA"},
        {9, "Data Context"},
        {10, "Invalidate Data Context"},
        {11, "Writeback Data Context"},
        {12, "AI_CPU"},
        {13, "Load Context"},
        {15, "DSA"}
};
}

std::string GetDeviceTaskTypeStr(const DeviceTask& task)
{
    std::string res = UNKNOWN_STRING;
    if (task.logType == HalLogType::ACSQ_LOG) {
        auto it = deviceTaskAcsqTypeMap.find(task.taskType);
        if (it != deviceTaskAcsqTypeMap.end()) {
            res = it->second;
        }
    } else if (task.logType == HalLogType::FFTS_LOG) {
        auto it = deviceTaskFftsPlusTypeMap.find(task.taskType);
        if (it != deviceTaskFftsPlusTypeMap.end()) {
            res = it->second;
        }
    }
    return res;
}

void MergeByHostAndDeviceTask(std::vector<TopDownTask>& res, HostTask& hostTask, std::vector<DeviceTask>& deviceTask)
{
    for (auto& task : deviceTask) {
        res.emplace_back(hostTask.taskId, hostTask.batchId, hostTask.streamId, hostTask.contextId, hostTask.requestId,
                         GetDeviceTaskTypeStr(task), hostTask.taskTypeStr, hostTask.modelId, hostTask.connection_id,
                         task.taskStart, task.taskEnd);
    }
}

void MergeByOnlyHostTask(std::vector<TopDownTask>& res, std::vector<HostTask>& hostTask)
{
    for (auto& task : hostTask) {
        res.emplace_back(task.taskId, task.batchId, task.streamId, task.contextId, task.requestId,
                         UNKNOWN_STRING, task.taskTypeStr, task.modelId, task.connection_id, DEFAULT_TIMESTAMP,
                         DEFAULT_TIMESTAMP);
    }
}

void MergeByOnlyDeviceTask(std::vector<TopDownTask>& res, std::vector<DeviceTask>& deviceTask, const TaskId& key)
{
    for (auto& task : deviceTask) {
        res.emplace_back(key.taskId, key.batchId, key.streamId, key.contextId, DEFAULT_INDEX_ID,
                         GetDeviceTaskTypeStr(task), UNKNOWN_STRING, DEFAULT_MODEL_ID,
                         DEFAULT_CONNECTION_ID, task.taskStart, task.taskEnd);
    }
}

std::vector<TopDownTask> GenerateTopDownTask(std::map<TaskId, std::vector<HostTask>>& hostTasks,
                                             std::map<TaskId, std::vector<DeviceTask>>& deviceTasks)
{
    std::set<TaskId> totalTaskId;
    std::transform(hostTasks.begin(), hostTasks.end(), std::inserter(totalTaskId, totalTaskId.begin()),
                   [](const std::pair<TaskId, std::vector<HostTask>>& p) { return p.first; });
    for (auto& it : deviceTasks) {
        totalTaskId.insert(it.first);
    }
    std::vector<TopDownTask> res;
    size_t matchSize = 0;
    for (auto& key : totalTaskId) {
        auto hostIt = hostTasks.find(key);
        auto deviceIt = deviceTasks.find(key);
        if (hostIt != hostTasks.end() && deviceIt != deviceTasks.end()) {
            if (hostIt->second.size() > 1 && deviceIt->second.size() > 1) {
                ERROR("There are % hostTask and % deviceTask, can't match streamId is: %, taskId is: %, ctxId is: %,"
                      "batchId is: %", hostIt->second.size(), deviceIt->second.size(), key.streamId, key.taskId,
                      key.contextId, key.batchId);
                continue;
            }
            if (hostIt->second.size() == 1) {
                MergeByHostAndDeviceTask(res, hostIt->second[0], deviceIt->second);
                matchSize += deviceIt->second.size();
            }
        } else if (hostIt == hostTasks.end()) {
            MergeByOnlyDeviceTask(res, deviceIt->second, key);
        } else { // 因为所有的key都是从hostTasks与deviceTasks中取的，所以一定不会有两者都找不到key的情况，该分支为device为空
            MergeByOnlyHostTask(res, hostIt->second);
        }
    }
    INFO("Found % host device matched task, generate % topDownTask", matchSize, res.size());
    return res;
}

uint32_t AscendTaskAssociation::ProcessEntry(DataInventory &dataInventory, const Context &context)
{
    auto hostTasks = dataInventory.GetPtr<std::map<TaskId, std::vector<HostTask>>>();
    auto deviceTasks = dataInventory.GetPtr<std::map<TaskId, std::vector<DeviceTask>>>();
    if (hostTasks->empty() && deviceTasks->empty()) {
        ERROR("There is no HostTask and DeviceTask, can't generate AscendTask!");
        return ANALYSIS_ERROR;
    }
    auto topDownTask = GenerateTopDownTask(*hostTasks, *deviceTasks);
    std::shared_ptr<std::vector<TopDownTask>> data;
    MAKE_SHARED_RETURN_VALUE(data, std::vector<TopDownTask>, ANALYSIS_ERROR, std::move(topDownTask));
    dataInventory.Inject(data);
    return ANALYSIS_OK;
}

REGISTER_PROCESS_SEQUENCE(AscendTaskAssociation, true, LoadHostData);
REGISTER_PROCESS_DEPENDENT_DATA(AscendTaskAssociation, std::map<TaskId, std::vector<DeviceTask>>,
                                std::map<TaskId, std::vector<HostTask>>);
REGISTER_PROCESS_SUPPORT_CHIP(AscendTaskAssociation, CHIP_ID_ALL);
}
}