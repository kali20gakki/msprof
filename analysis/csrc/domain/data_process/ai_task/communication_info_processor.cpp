/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_info_processor.cpp
 * Description        : communication_info_processor处理HCCLTaskSingleDevice,HCCLOpSingleDevice表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/08/03
 * *****************************************************************************
 */
#include "communication_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"


namespace Analysis {
namespace Domain {
using namespace Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct CommunicationOpEndpointsTime {
    double firstTaskStartTime;
    double lastTaskStartTime;
    double lastTaskDuration;
};

// groupName 依据hash进行转换，对于无hash的数据，直接取用hash值（即groupName）进行转换
std::string GetGroupNameValue(const std::string& groupName, GeHashMap& hashMap)
{
    if (groupName != NA && Utils::IsNumber(groupName)) {
        if (hashMap.find(groupName) != hashMap.end()) {
            return hashMap[groupName];
        }
    }
    return groupName;
}
}

CommunicationInfoProcessor::CommunicationInfoProcessor(const std::string& profPaths)
    : DataProcessor(profPaths)
{}

bool CommunicationInfoProcessor::Process(DataInventory& dataInventory)
{
    CommunicationData communicationData;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    bool flag = true;
    for (const auto& devicePath : deviceList) {
        communicationData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetProfTimeRecordInfo(communicationData.timeRecord, profPath_,
                                                          communicationData.deviceId)) {
            ERROR("Failed to obtain the time in start_info and end_info. "
                  "Path is %, device id is %.", profPath_, communicationData.deviceId);
            flag = false;
        }
        if (!ProcessOneDevice(devicePath, communicationData, dataInventory)) {
            flag = false;
        }
    }
    if (!SaveToDataInventory<CommunicationTaskData>(std::move(communicationData.resTaskData), dataInventory,
                                                    TABLE_NAME_COMMUNICATION_TASK_INFO)) {
        ERROR("Save data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        return false;
    }
    if (!SaveToDataInventory<CommunicationOpData>(std::move(communicationData.resOpData), dataInventory,
                                                  TABLE_NAME_COMMUNICATION_OP)) {
        ERROR("Save data failed, %.", TABLE_NAME_COMMUNICATION_OP);
        return false;
    }
    return flag;
}

OriTaskDataFormat CommunicationInfoProcessor::LoadTaskData(const DBInfo& taskSingleDevice)
{
    OriTaskDataFormat oriTaskData;
    std::string sql{"SELECT model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
                    "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
                    "rdma_type, timestamp, duration, connection_id, duration_estimated, bandwidth "
                    "FROM " + taskSingleDevice.tableName};
    if (!taskSingleDevice.dbRunner->QueryData(sql, oriTaskData)) {
        ERROR("Failed to obtain data from the % table.", taskSingleDevice.tableName);
    }
    return oriTaskData;
}

OriOpDataFormat CommunicationInfoProcessor::LoadOpData(const DBInfo& opSingleDevice)
{
    OriOpDataFormat oriOpData;
    std::string sql{"SELECT connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type, "
                    "model_id FROM " + opSingleDevice.tableName};
    if (!opSingleDevice.dbRunner->QueryData(sql, oriOpData)) {
        ERROR("Failed to obtain data from the % table.", opSingleDevice.tableName);
    }
    return oriOpData;
}

bool CommunicationInfoProcessor::FormatData(std::vector<CommunicationTaskData>& taskFormatData,
                                            std::vector<CommunicationOpData>& opFormatData,
                                            CommunicationData& communicationData)
{
    if (!Utils::Reserve(taskFormatData, communicationData.oriTaskData.size())) {
        ERROR("Reserve for communication task data failed.");
        return false;
    }
    CommunicationTaskData taskData;
    HcclTaskSingleDeviceData hcclData;
    std::unordered_map<std::string, CommunicationOpData> opDataMap;
    std::unordered_map<std::string, CommunicationOpEndpointsTime> endpoints;
    std::unordered_map<uint32_t, size_t> opInfoIdxMap = GenOpInfoIdxMap(communicationData.oriOpData);
    for (auto& row : communicationData.oriTaskData) {
        Update(row, hcclData, taskData, communicationData);
        taskFormatData.push_back(taskData);
        if (opDataMap.find(taskData.opKey) == opDataMap.end()) {
            opDataMap[taskData.opKey].opKey = taskData.opKey;
            opDataMap[taskData.opKey].opName = taskData.opName;
            endpoints[taskData.opKey].firstTaskStartTime = hcclData.timestamp;
            endpoints[taskData.opKey].lastTaskStartTime = hcclData.timestamp;
            endpoints[taskData.opKey].lastTaskDuration = hcclData.duration;
            opDataMap[taskData.opKey].connectionId = hcclData.connectionId;
            opDataMap[taskData.opKey].groupName = taskData.groupName;
            UpdateOpInfo(opDataMap[taskData.opKey], hcclData.connectionId, opInfoIdxMap, communicationData.oriOpData,
                         communicationData);
        } else {
            endpoints[taskData.opKey].firstTaskStartTime = std::min(endpoints[taskData.opKey].firstTaskStartTime,
                                                                    hcclData.timestamp);
            if (hcclData.timestamp + hcclData.duration > endpoints[taskData.opKey].lastTaskStartTime +
                endpoints[taskData.opKey].lastTaskDuration) {
                endpoints[taskData.opKey].lastTaskStartTime = hcclData.timestamp;
                endpoints[taskData.opKey].lastTaskDuration = hcclData.duration;
            }
        }
    }
    for (auto item = opDataMap.begin(); item != opDataMap.end(); ++item) {
        auto key = item->first;
        auto data = item->second;
        HPFloat start{endpoints[key].firstTaskStartTime};
        HPFloat end = HPFloat(endpoints[key].lastTaskStartTime) + HPFloat(endpoints[key].lastTaskDuration);
        data.start = Utils::GetLocalTime(start, communicationData.timeRecord).Uint64();
        data.end = Utils::GetLocalTime(end, communicationData.timeRecord).Uint64();
        opFormatData.push_back(data);
    }
    return true;
}

void CommunicationInfoProcessor::Update(const HcclTaskFormat& oriData, HcclTaskSingleDeviceData& hcclData,
                                        CommunicationTaskData& taskData, CommunicationData& communicationData)
{
    std::tie(taskData.modelId, hcclData.opName, hcclData.HCCLName, hcclData.groupName, taskData.planeId,
             taskData.streamId, taskData.taskId, taskData.srcRank, taskData.dstRank,
             hcclData.transportType, taskData.size, hcclData.dataType, hcclData.linkType,
             taskData.contextId, taskData.notifyId, taskData.batchId, hcclData.rdmaType,
             hcclData.timestamp, hcclData.duration, hcclData.connectionId,
             taskData.durationEstimated, taskData.bandwidth) = oriData;
    taskData.opName = hcclData.opName;
    taskData.deviceId = communicationData.deviceId;
    taskData.taskType = hcclData.HCCLName;
    taskData.duration = hcclData.duration;
    HPFloat timestamp{hcclData.timestamp};
    taskData.start = GetLocalTime(timestamp, communicationData.timeRecord).Uint64();
    taskData.groupName = GetGroupNameValue(hcclData.groupName, communicationData.hashMap);
    taskData.rdmaType = GetEnumTypeValue(hcclData.rdmaType, NAME_STR(HCCL_RDMA_TYPE_TABLE), HCCL_RDMA_TYPE_TABLE);
    taskData.transportType = GetEnumTypeValue(hcclData.transportType,
                                              NAME_STR(HCCL_TRANSPORT_TYPE_TABLE), HCCL_TRANSPORT_TYPE_TABLE);
    taskData.dataType = GetEnumTypeValue(hcclData.dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
    taskData.linkType = GetEnumTypeValue(hcclData.linkType, NAME_STR(HCCL_LINK_TYPE_TABLE), HCCL_LINK_TYPE_TABLE);
    taskData.opKey = Utils::Join("_", hcclData.opName, hcclData.groupName, communicationData.deviceId);
}

void CommunicationInfoProcessor::UpdateOpInfo(CommunicationOpData& opData, uint32_t connectionId,
                                              const std::unordered_map<uint32_t, size_t>& opInfoIdxMap,
                                              const OriOpDataFormat& oriOpData, CommunicationData& communicationData)
{
    auto indexIt = opInfoIdxMap.find(connectionId);
    if (indexIt != opInfoIdxMap.end()) {
        const auto& oriData = oriOpData[indexIt->second];
        std::string opName;
        std::string dataType;
        std::string algType;
        std::string groupName;
        std::string opType;
        std::tie(connectionId, opName, opData.relay, opData.retry, dataType, algType,
                 opData.count, groupName, opType, opData.modelId) = oriData;
        opData.dataType = GetEnumTypeValue(dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
        opData.algType = algType;
        opData.groupName = GetGroupNameValue(groupName, communicationData.hashMap);
        opData.opType = opType;
        opData.deviceId = communicationData.deviceId;
    }
}

std::unordered_map<uint32_t, size_t> CommunicationInfoProcessor::GenOpInfoIdxMap(const OriOpDataFormat& oriOpData)
{
    std::unordered_map<uint32_t, size_t> opInfoIdxMap;
    uint32_t connectionId = 0;
    for (size_t i = 0; i < oriOpData.size(); ++i) {
        connectionId = std::get<0>(oriOpData[i]);
        opInfoIdxMap[connectionId] = i;
    }
    return opInfoIdxMap;
}

bool CommunicationInfoProcessor::ProcessOneDevice(const std::string& devicePath, CommunicationData& communicationData,
                                                  DataInventory& dataInventory)
{
    DBInfo taskDBInfo("hccl_single_device.db", "HCCLTaskSingleDevice");
    DBInfo opDBInfo("hccl_single_device.db", "HCCLOpSingleDevice");
    std::string taskDBPath = Utils::File::PathJoin({devicePath, SQLITE, taskDBInfo.dbName});
    std::string opDBPath = Utils::File::PathJoin({devicePath, SQLITE, opDBInfo.dbName});
    if (!taskDBInfo.ConstructDBRunner(taskDBPath) || !opDBInfo.ConstructDBRunner(opDBPath)) {
        return false;
    }
    auto status = CheckPathAndTable(taskDBPath, taskDBInfo, false);
    if (status != CHECK_SUCCESS) {
        return status != CHECK_FAILED;
    }
    status = CheckPathAndTable(opDBPath, opDBInfo, false);
    if (status != CHECK_SUCCESS) {
        return status != CHECK_FAILED;
    }
    auto hashMap = dataInventory.GetPtr<GeHashMap>();
    if (hashMap == nullptr) {
        ERROR("Can't get hash data.");
        return false;
    }
    communicationData.hashMap = *hashMap;
    communicationData.oriTaskData = LoadTaskData(taskDBInfo);
    if (communicationData.oriTaskData.empty()) {
        ERROR("Get % data failed in %.", taskDBInfo.tableName, taskDBPath);
        return false;
    }
    communicationData.oriOpData = LoadOpData(opDBInfo);
    if (communicationData.oriOpData.empty()) {
        ERROR("Get % data failed in %.", opDBInfo.tableName, opDBPath);
        return false;
    }
    std::vector<CommunicationTaskData> taskData;
    std::vector<CommunicationOpData> opData;
    if (!FormatData(taskData, opData, communicationData)) {
        ERROR("Format data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        return false;
    }
    communicationData.resTaskData.insert(communicationData.resTaskData.end(), taskData.begin(), taskData.end());
    communicationData.resOpData.insert(communicationData.resOpData.end(), opData.begin(), opData.end());
    return true;
}
} // Domain
} // Analysis
