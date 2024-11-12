/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_hardware_assembler.cpp
 * Description        : 组合Ascend Hardware层数据
 * Author             : msprof team
 * Creation Date      : 2024/8/27
 * *****************************************************************************
 */

#include "analysis/csrc/application/timeline/ascend_hardware_assembler.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/application/timeline/connection_id_pool.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Viewer::Database;
namespace {
const uint32_t SORT_INDEX = 13;
const std::string ASCEND_ASSEMBLER = "Ascend Hardware";
const std::string ASCEND_LABEL = "NPU";
const std::string TASK_TYPE_FFTS_PLUS = "FFTS_PLUS";
const std::string TASK_TYPE_UNKNOWN = "UNKNOWN";
}

AscendHardwareAssembler::AscendHardwareAssembler()
    : JsonAssembler(PROCESS_TASK, {{MSPROF_JSON_FILE, FileCategory::MSPROF}}) {}

void TaskTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["Model Id"] << modelId_;
    ostream["Task Type"] << taskType_;
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
    ostream["Batch Id"] << batchId_;
    ostream["Subtask Id"] << contextId_;
    ostream["connection_id"] << connectionId_;
}

void DeviceTxTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
}

void KfcTurnTraceEvent::ProcessArgs(JsonWriter& ostream)
{
    ostream["Physic Stream Id"] << streamId_;
    ostream["Task Id"] << taskId_;
}

void AscendHardwareAssembler::InitData(DataInventory &dataInventory, std::vector<AscendTaskData> &taskData)
{
    logicStream_ = dataInventory.GetPtr<std::unordered_map<uint32_t, uint32_t>>();
    auto taskInfo = dataInventory.GetPtr<std::vector<TaskInfoData>>();
    if (taskInfo != nullptr) {
        for (const auto &node : *taskInfo) {
            opName_.emplace(TaskId{static_cast<uint16_t >(node.streamId), static_cast<uint16_t >(node.batchId),
                static_cast<uint16_t >(node.taskId), node.contextId, node.deviceId}, node.opName);
        }
    }
    for (const auto& data : taskData) {
        if (data.contextId != UINT32_MAX) {
            ffts_.emplace(TaskId{static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId),
                                 static_cast<uint16_t>(data.taskId), UINT32_MAX, data.deviceId});
        }
    }
}

std::string AscendHardwareAssembler::GetOpName(const AscendTaskData& data)
{
    TaskId id{static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId),
        static_cast<uint16_t>(data.taskId), data.contextId, data.deviceId};
    auto it = opName_.find(id);
    if (it != opName_.end()) {
        return it->second;
    }
    if (data.hostType == TASK_TYPE_FFTS_PLUS || data.hostType == TASK_TYPE_UNKNOWN) {
        return data.deviceType;
    }
    return data.hostType;
}

uint32_t AscendHardwareAssembler::GetPhysicStreamId(const uint32_t streamId)
{
    if (logicStream_ == nullptr) {
        return streamId;
    }
    auto it = logicStream_->find(streamId);
    if (it != logicStream_->end()) {
        return it->second;
    }
    return streamId;
}

void AscendHardwareAssembler::GenerateTaskTrace(const std::vector<AscendTaskData> &taskData,
                                                const std::string &profPath,
                                                std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    std::string traceName;
    TaskId id;
    for (const auto &data : taskData) {
        id = {static_cast<uint16_t>(data.streamId), static_cast<uint16_t>(data.batchId),
              static_cast<uint16_t>(data.taskId), data.contextId, data.deviceId};
        if (ffts_.find(id) != ffts_.end()) { // 当前task存在ffts+任务，只呈现ffts+任务即可
            continue;
        }
        traceName = GetOpName(data);
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, SORT_INDEX);
        int tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        // 存储pid，tid组合的最小集
        pidTidSet_.insert({formatPid, tid});
        std::shared_ptr<TaskTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, TaskTraceEvent, formatPid, tid, data.duration / NS_TO_US,
                                std::to_string(data.start / NS_TO_US), traceName, data.modelId, data.streamId,
                                data.taskId, data.batchId, data.contextId, data.connectionId, data.deviceType);
        res_.push_back(event);
        GenerateTaskConnectionTrace(data, formatPid, id);
    }
}

void AscendHardwareAssembler::GenerateTxTrace(const std::vector<MsprofTxDeviceData>& txData,
                                              const std::string& profPath,
                                              std::unordered_map<uint16_t, uint32_t>& pidMap)
{
    uint32_t formatPid;
    std::string traceName;
    for (const auto &data : txData) {
        traceName = MS_TX;
        traceName.append("_").append(std::to_string(data.indexId));
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, SORT_INDEX);
        int tid = static_cast<int>(data.streamId);
        // 存储pid，tid组合的最小集
        pidTidSet_.insert({formatPid, tid});
        std::shared_ptr<DeviceTxTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, DeviceTxTraceEvent, formatPid, tid, data.duration / NS_TO_US,
                                std::to_string(data.start / NS_TO_US), traceName, data.streamId, data.taskId);
        res_.push_back(event);
    }
}

void AscendHardwareAssembler::GenerateKfcTrace(const std::vector<KfcTurnData> &kfcData, const std::string &profPath,
                                               std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    for (const auto &datum: kfcData) {
        std::string traceName = datum.opName;
        formatPid = GetDevicePid(pidMap, datum.deviceId, profPath, SORT_INDEX);
        int formatTid = static_cast<int>(GetPhysicStreamId(datum.streamId));
        // 存储pid，tid组合的最小集
        pidTidSet_.insert({formatPid, formatTid});
        std::shared_ptr<KfcTurnTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, KfcTurnTraceEvent, formatPid, formatTid, datum.duration / NS_TO_US,
                                std::to_string(datum.startTime / NS_TO_US), traceName, datum.streamId, datum.taskId);
        res_.push_back(event);
    }
}

void AscendHardwareAssembler::GenerateTaskConnectionTrace(const AscendTaskData &data, uint32_t formatPid, TaskId &id)
{
    std::string connId;
    std::string name;
    int tid;
    if (opName_.find(id) != opName_.end()) {
        connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
        name = HOST_TO_DEVICE + connId;
        tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        std::shared_ptr<FlowEvent> end;
        MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid, std::to_string(data.start / NS_TO_US),
                                HOST_TO_DEVICE, connId, name, FLOW_END, FLOW_BP);
        res_.push_back(end);
    }
}

void AscendHardwareAssembler::GenerateTxConnectionTrace(const std::vector<MsprofTxDeviceData> &txData,
                                                        std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    std::string connId;
    uint32_t formatPid;
    int tid;
    for (const auto &data : txData) {
        connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::MSPROF_TX);
        auto name = MS_TX;
        name.append("_").append(connId);
        formatPid = pidMap[data.deviceId];
        tid = static_cast<int>(data.streamId);
        std::shared_ptr<FlowEvent> end;
        MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid, std::to_string(data.start / NS_TO_US),
                                MS_TX, connId, name, FLOW_END, FLOW_BP);
        res_.push_back(end);
    }
}

void AscendHardwareAssembler::GenerateMetaData(std::unordered_map<uint16_t, uint32_t>& pidMap)
{
    for (const auto &it : pidMap) {
        std::shared_ptr<MetaDataNameEvent> processName;
        MAKE_SHARED_RETURN_VOID(processName, MetaDataNameEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_NAME,
                                ASCEND_ASSEMBLER);
        res_.push_back(processName);
        std::shared_ptr<MetaDataLabelEvent> processLabel;
        MAKE_SHARED_RETURN_VOID(processLabel, MetaDataLabelEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_LABEL,
                                ASCEND_LABEL);
        res_.push_back(processLabel);
        std::shared_ptr<MetaDataIndexEvent> processIndex;
        MAKE_SHARED_RETURN_VOID(processIndex, MetaDataIndexEvent, it.second, DEFAULT_TID, META_DATA_PROCESS_INDEX,
                                SORT_INDEX);
        res_.push_back(processIndex);
    }
    for (const auto &it : pidTidSet_) {
        std::string argName{"Stream " + std::to_string(it.second)};
        std::shared_ptr<MetaDataNameEvent> threadName;
        MAKE_SHARED_RETURN_VOID(threadName, MetaDataNameEvent, it.first, it.second, META_DATA_THREAD_NAME, argName);
        res_.push_back(threadName);
        std::shared_ptr<MetaDataIndexEvent> threadIndex;
        MAKE_SHARED_RETURN_VOID(threadIndex, MetaDataIndexEvent, it.first, it.second, META_DATA_THREAD_INDEX,
                                it.second);
        res_.push_back(threadIndex);
    }
}

uint8_t  AscendHardwareAssembler::AssembleData(DataInventory& dataInventory, JsonWriter& ostream,
                                               const std::string& profPath)
{
    auto taskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    auto deviceTxData = dataInventory.GetPtr<std::vector<MsprofTxDeviceData>>();
    auto kfcTurnData = dataInventory.GetPtr<std::vector<KfcTurnData>>();
    if (taskData == nullptr && deviceTxData == nullptr && kfcTurnData == nullptr) {
        WARN("Can't get task data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    if (taskData != nullptr) {
        InitData(dataInventory, *taskData);
        GenerateTaskTrace(*taskData, profPath, devicePid);
    }
    if (deviceTxData != nullptr) {
        GenerateTxTrace(*deviceTxData, profPath, devicePid);
        GenerateTxConnectionTrace(*deviceTxData, devicePid);
    }
    if (kfcTurnData != nullptr) {
        GenerateKfcTrace(*kfcTurnData, profPath, devicePid);
    }
    GenerateMetaData(devicePid);
    if (res_.empty()) {
        ERROR("Can't Generate any Ascend process data");
        return ASSEMBLE_FAILED;
    }
    for (const auto &node : res_) {
        node->DumpJson(ostream);
    }
    // 为了让下一个写入的内容形成正确的JSON格式，需要补一个","
    ostream << ",";
    return ASSEMBLE_SUCCESS;
}
}
}
