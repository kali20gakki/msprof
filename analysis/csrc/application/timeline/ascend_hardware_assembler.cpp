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
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Viewer::Database;
namespace {
const std::string TASK_TYPE_FFTS_PLUS = "FFTS_PLUS";
const std::string TASK_TYPE_UNKNOWN = "UNKNOWN";
const int NS_TO_US_INT = 1000;
const int HUNDRED_DIGIT_FLAG = 100;
const int TEN_DIGIT_FLAG = 100;
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
    auto apiData = dataInventory.GetPtr<std::vector<ApiData>>();
    if (apiData != nullptr) {
        for (const auto &node : *apiData) {
            if (RECORD_EVENT == node.id) {
                recordEvent_.emplace(node.connectionId);
            }
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

std::string AscendHardwareAssembler::ConvertNs2UsWithDouble(uint64_t value)
{
    // 提取整数部分和小数部分
    uint64_t integer_part = value / NS_TO_US_INT;
    uint64_t fractional_part = value % NS_TO_US_INT;
    // 构造字符串结果
    std::string result = std::to_string(integer_part) + "." + (fractional_part < HUNDRED_DIGIT_FLAG ? "0" : "") +
            (fractional_part < TEN_DIGIT_FLAG ? "0" : "") + std::to_string(fractional_part);

    return result;
}

void AscendHardwareAssembler::GenerateTaskTrace(const std::vector<AscendTaskData> &taskData,
                                                const std::string &profPath, const LayerInfo &layer,
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
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        // 存储pid，tid组合的最小集
        pidTidSet_.insert({formatPid, tid});
        std::shared_ptr<TaskTraceEvent> event;
        MAKE_SHARED_RETURN_VOID(event, TaskTraceEvent, formatPid, tid, data.duration / NS_TO_US,
                                ConvertNs2UsWithDouble(data.start), traceName,
                                data.modelId, data.streamId,
                                data.taskId, data.batchId, data.contextId, data.connectionId, data.deviceType);
        res_.push_back(event);
        GenerateTaskConnectionTrace(data, formatPid, id);
    }
}

void AscendHardwareAssembler::GenerateKfcTrace(const std::vector<KfcTurnData> &kfcData, const std::string &profPath,
                                               const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    for (const auto &datum: kfcData) {
        std::string traceName = datum.opName;
        formatPid = GetDevicePid(pidMap, datum.deviceId, profPath, layer.sortIndex);
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
    if (opName_.find(id) != opName_.end() || recordEvent_.find(data.connectionId) != recordEvent_.end()) {
        connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
        name = HOST_TO_DEVICE + connId;
        tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        std::shared_ptr<FlowEvent> end;
        MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid,  ConvertNs2UsWithDouble(data.start),
                                HOST_TO_DEVICE, connId, name, FLOW_END, FLOW_BP);
        res_.push_back(end);
    }
}

uint8_t  AscendHardwareAssembler::AssembleData(DataInventory& dataInventory, JsonWriter& ostream,
                                               const std::string& profPath)
{
    auto taskData = dataInventory.GetPtr<std::vector<AscendTaskData>>();
    auto kfcTurnData = dataInventory.GetPtr<std::vector<KfcTurnData>>();
    if (taskData == nullptr && kfcTurnData == nullptr) {
        WARN("Can't get task data from dataInventory");
        return DATA_NOT_EXIST;
    }
    std::unordered_map<uint16_t, uint32_t> devicePid;
    auto layer = GetLayerInfo(PROCESS_TASK);
    if (taskData != nullptr) {
        InitData(dataInventory, *taskData);
        GenerateTaskTrace(*taskData, profPath, layer, devicePid);
    }
    if (kfcTurnData != nullptr) {
        GenerateKfcTrace(*kfcTurnData, profPath, layer, devicePid);
    }
    GenerateTaskMetaData(devicePid, layer, res_, pidTidSet_);
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
