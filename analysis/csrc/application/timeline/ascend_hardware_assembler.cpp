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
#include "analysis/csrc/application/timeline/connection_id_pool.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Viewer::Database;
using namespace Analysis::Utils;
namespace {
struct MemcpyRecord {
    uint64_t dataSize;  // memcpy的数据量
    uint64_t maxSize;  // 每个task能拷贝的最大数据量
    uint64_t remainSize;  // 还剩下多少数据量需要后面的task去拷贝
    std::string memcpyDirection;
};
using MEMCPY_ASYNC_FORMAT = std::unordered_map<uint64_t, MemcpyRecord>;
const std::string TASK_TYPE_FFTS_PLUS = "FFTS_PLUS";
const std::string TASK_TYPE_UNKNOWN = "UNKNOWN";
}

MEMCPY_ASYNC_FORMAT GetMemcpyRecordMap(DataInventory& dataInventory)
{
    MEMCPY_ASYNC_FORMAT memcpyRecordMap;
    auto memcpyInfoData = dataInventory.GetPtr<std::vector<MemcpyInfoData>>();
    if (memcpyInfoData == nullptr) {
        WARN("Can't get memcpyInfo data from dataInventory");
        return memcpyRecordMap;
    }
    for (const auto& data: *memcpyInfoData) {
        memcpyRecordMap[data.connectionId] = {data.dataSize, data.maxSize, data.dataSize, data.memcpyDirection};
    }
    return memcpyRecordMap;
}

MemcpyRecord GenerateMemcpyRecordByConnectionId(MEMCPY_ASYNC_FORMAT &memcpyRecordMap, const uint64_t &connectionId)
{
    MemcpyRecord tmpMemcpyRecord {0, 0, 0, "other"};
    auto it = memcpyRecordMap.find(connectionId);
    if (it == memcpyRecordMap.end()) {
        ERROR("MEMCPY_ASYNC task lost memcpyInfo, connectionId is %", connectionId);
    } else {
        if (it->second.remainSize == 0) {
            ERROR("Redundant memcpyAsync task, connectionId is %", connectionId);
        } else if (it->second.remainSize <= it->second.maxSize) {
            tmpMemcpyRecord.dataSize = it->second.remainSize;
            it->second.remainSize = 0;
        } else if (it->second.remainSize > it->second.maxSize) {
            tmpMemcpyRecord.dataSize = it->second.maxSize;
            it->second.remainSize = it->second.remainSize - tmpMemcpyRecord.dataSize;
        }
        tmpMemcpyRecord.memcpyDirection = it->second.memcpyDirection;
    }
    return tmpMemcpyRecord;
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

void MemcpyAsyncEvent::ProcessArgs(JsonWriter& ostream)
{
    TaskTraceEvent::ProcessArgs(ostream);
    if (showFlag_) {
        ostream["dataSize(B)"] << dataSize_;
        ostream["bandwidth(GB/s)"] << bandwidth_;
        ostream["direction"] << memcpyDirection_;
    }
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
        if (data.hostType == MEMCPY_ASYNC) {
            memcpyAsyncConnectionIds_.emplace(data.connectionId);
            memcpyAsyncDeviceTasks_.push_back(data);
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
                                                const std::string &profPath, const LayerInfo &layer,
                                                std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    std::string traceName;
    TaskId id;
    for (const auto &data : taskData) {
        if (memcpyAsyncConnectionIds_.find(data.connectionId) != memcpyAsyncConnectionIds_.end()) {
            continue;  // MEMCPY_ASYNC类型的task有新增args,需要单独处理
        }
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
                                DivideByPowersOfTenWithPrecision(data.start), traceName,
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
                                DivideByPowersOfTenWithPrecision(datum.startTime),
                                traceName, datum.streamId, datum.taskId);
        res_.push_back(event);
    }
}

void AscendHardwareAssembler::GenerateMemcpyAsyncTrace(DataInventory &dataInventory, const std::string &profPath,
    const LayerInfo &layer, std::unordered_map<uint16_t, uint32_t> &pidMap)
{
    uint32_t formatPid;
    std::string traceName;
    uint64_t dataSize;
    double bandwidth;
    std::string memcpyDirection;
    bool showFlag = true;
    // 为了处理Memcpy数据量过大时rutnime track会拆分成多条，而memcpy info只有一条的情况
    MEMCPY_ASYNC_FORMAT memcpyRecordMap = GetMemcpyRecordMap(dataInventory);
    if (memcpyRecordMap.empty()) {
        showFlag = false;
    }
    auto comp =
    [](AscendTaskData &data1, AscendTaskData &data2) {
        return data1.start < data2.start;
    };
    std::sort(memcpyAsyncDeviceTasks_.begin(), memcpyAsyncDeviceTasks_.end(), comp);  // 按照时间戳排序
    for (const auto &data : memcpyAsyncDeviceTasks_) {
        formatPid = GetDevicePid(pidMap, data.deviceId, profPath, layer.sortIndex);
        int tid = static_cast<int>(GetPhysicStreamId(data.streamId));
        // 存储pid，tid组合的最小集
        pidTidSet_.insert({formatPid, tid});
        std::shared_ptr<MemcpyAsyncEvent> event;
        // 计算拷贝数据量和带宽
        if (showFlag) {
            MemcpyRecord tmpMemcpyRecord = GenerateMemcpyRecordByConnectionId(memcpyRecordMap, data.connectionId);
            dataSize = tmpMemcpyRecord.dataSize;
            memcpyDirection = tmpMemcpyRecord.memcpyDirection;
            if (data.duration > 0) {
                bandwidth = static_cast<double>(tmpMemcpyRecord.dataSize) / data.duration;  // GB/s, 全部按照1000计算
            }
        }
        MAKE_SHARED_RETURN_VOID(event, MemcpyAsyncEvent, formatPid, tid, data.duration / NS_TO_US,
                                DivideByPowersOfTenWithPrecision(data.start), data.hostType,
                                data.modelId, data.streamId, data.taskId, data.batchId, data.contextId,
                                data.connectionId, data.deviceType, dataSize, bandwidth, memcpyDirection, showFlag);
        res_.push_back(event);
        GenerateMemcpyAsyncConnectionTrace(data, formatPid);
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
        MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid,  DivideByPowersOfTenWithPrecision(data.start),
                                HOST_TO_DEVICE, connId, name, FLOW_END, FLOW_BP);
        res_.push_back(end);
    }
}

void AscendHardwareAssembler::GenerateMemcpyAsyncConnectionTrace(const AscendTaskData &data, uint32_t formatPid)
{
    std::string connId = ConnectionIdPool::GetConnectionId(data.connectionId, ConnectionCategory::GENERAL);
    std::string name = HOST_TO_DEVICE + connId;
    int tid = static_cast<int>(GetPhysicStreamId(data.streamId));
    std::shared_ptr<FlowEvent> end;
    MAKE_SHARED_RETURN_VOID(end, FlowEvent, formatPid, tid,  DivideByPowersOfTenWithPrecision(data.start),
                            HOST_TO_DEVICE, connId, name, FLOW_END, FLOW_BP);
    res_.push_back(end);
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
        if (!memcpyAsyncDeviceTasks_.empty()) {
            GenerateMemcpyAsyncTrace(dataInventory, profPath, layer, devicePid);
        }
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
