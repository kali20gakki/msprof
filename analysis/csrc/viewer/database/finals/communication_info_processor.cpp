/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_info_processor.cpp
 * Description        : communication_info_processor，处理表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/communication_info_processor.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;
using namespace Parser::Environment;
using namespace Analysis::Utils;
namespace {
const std::string NA = "N/A";
const std::string INDEX_NAME = "CommunicationTaskIndex";
const std::vector<std::string> COMMUNICATION_TASK_INDEX_COL_NAMES = {"globalTaskId"};
struct CommunicationOpEndpointsTime {
    double firstTaskStartTime;
    double lastTaskStartTime;
    double lastTaskDuration;
};

uint64_t GetGroupNameValue(const std::string &groupName, GeHashMap &hashMap)
{
    uint64_t intGroupName = 0;
    if (groupName != NA && Utils::StrToU64(intGroupName, groupName) == ANALYSIS_OK) {
        return IdPool::GetInstance().GetUint64Id(hashMap[groupName]);
    }
    return IdPool::GetInstance().GetUint64Id(groupName);
}
}

CommunicationInfoProcessor::CommunicationInfoProcessor(const std::string &msprofDBPath,
                                                       const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool CommunicationInfoProcessor::Run()
{
    INFO("CommunicationInfoProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_COMMUNICATION);
    return flag;
}

CommunicationInfoProcessor::OriTaskDataFormat CommunicationInfoProcessor::GetTaskData(const DBInfo &taskSingleDevice)
{
    OriTaskDataFormat oriTaskData;
    std::string sql{"SELECT model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
                    "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
                    "rdma_type, timestamp, duration, connection_id FROM " + taskSingleDevice.tableName};
    if (!taskSingleDevice.dbRunner->QueryData(sql, oriTaskData)) {
        ERROR("Failed to obtain data from the % table.", taskSingleDevice.tableName);
    }
    return oriTaskData;
}

CommunicationInfoProcessor::OriOpDataFormat CommunicationInfoProcessor::GetOpData(const DBInfo &opSingleDevice)
{
    OriOpDataFormat oriOpData;
    std::string sql{"SELECT connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type "
                    "FROM " + opSingleDevice.tableName};
    if (!opSingleDevice.dbRunner->QueryData(sql, oriOpData)) {
        ERROR("Failed to obtain data from the % table.", opSingleDevice.tableName);
    }
    return oriOpData;
}

bool CommunicationInfoProcessor::FormatData(const OriTaskDataFormat &oriTaskData, const OriOpDataFormat &oriOpData,
                                            CommunicationTaskDataFormat &processedTaskData,
                                            CommunicationOpDataFormat &processedOpData,
                                            const ThreadData &threadData, GeHashMap &hashMap)
{
    if (!Utils::Reserve(processedTaskData, oriTaskData.size())) {
        ERROR("Reserve for communication task data failed.");
        return false;
    }
    CommunicationTaskData taskData;
    HcclTaskSingleDeviceData hcclData;
    std::unordered_map<uint32_t, CommunicationOpData> opData;
    std::unordered_map<uint32_t, CommunicationOpEndpointsTime> endpoints;
    std::unordered_map<uint32_t, size_t> opInfoIdxMap = GenOpInfoIdxmap(oriOpData);
    for (auto &row: oriTaskData) {
        Update(row, hcclData, taskData, threadData.deviceId, hashMap);
        processedTaskData.emplace_back(taskData.opName, taskData.globalTaskId, taskData.taskType, taskData.planeId,
                                       taskData.groupName, taskData.notifyId, taskData.rdmaType, taskData.srcRank,
                                       taskData.dstRank, taskData.transportType, taskData.size, taskData.dataType,
                                       taskData.linkType, taskData.opId);
        if (opData.find(taskData.opId) == opData.end()) {
            opData[taskData.opId].opId = taskData.opId;
            opData[taskData.opId].opName = taskData.opName;
            endpoints[taskData.opId].firstTaskStartTime = hcclData.timestamp;
            endpoints[taskData.opId].lastTaskStartTime = hcclData.timestamp;
            endpoints[taskData.opId].lastTaskDuration = hcclData.duration;
            opData[taskData.opId].connectionId = Utils::Contact(threadData.profId, hcclData.connectionId);
            opData[taskData.opId].groupName = taskData.groupName;
            UpdateOpInfo(opData[taskData.opId], hcclData.connectionId, opInfoIdxMap, oriOpData, hashMap);
        } else {
            endpoints[taskData.opId].firstTaskStartTime = std::min(endpoints[taskData.opId].firstTaskStartTime,
                                                                   hcclData.timestamp);
            if (hcclData.timestamp + hcclData.duration > endpoints[taskData.opId].lastTaskStartTime +
                endpoints[taskData.opId].lastTaskDuration) {
                endpoints[taskData.opId].lastTaskStartTime = hcclData.timestamp;
                endpoints[taskData.opId].lastTaskDuration = hcclData.duration;
            }
        }
    }
    for (auto item = opData.begin(); item != opData.end(); ++item) {
        auto key = item->first;
        auto data = item->second;
        HPFloat start{endpoints[key].firstTaskStartTime};
        HPFloat end = HPFloat(endpoints[key].lastTaskStartTime) + HPFloat(endpoints[key].lastTaskDuration);
        data.start = Utils::GetLocalTime(start, threadData.timeRecord).Uint64();
        data.end = Utils::GetLocalTime(end, threadData.timeRecord).Uint64();
        processedOpData.emplace_back(data.opName, data.start, data.end, data.connectionId, data.groupName, data.opId,
                                     data.relay, data.retry, data.dataType, data.algType, data.count, data.opType);
    }
    return true;
}

void CommunicationInfoProcessor::Update(const HcclTaskFormat &oriData, HcclTaskSingleDeviceData &hcclData,
                                        CommunicationTaskData &taskData, uint16_t deviceId, GeHashMap &hashMap)
{
    std::tie(taskData.modelId, hcclData.opName, hcclData.HCCLName, hcclData.groupName, taskData.planeId,
             taskData.streamId, taskData.taskId, taskData.srcRank, taskData.dstRank,
             hcclData.transportType, taskData.size, hcclData.dataType, hcclData.linkType,
             taskData.contextId, taskData.notifyId, taskData.batchId, hcclData.rdmaType,
             hcclData.timestamp, hcclData.duration, hcclData.connectionId) = oriData;
    taskData.opName = IdPool::GetInstance().GetUint64Id(hcclData.opName);
    taskData.globalTaskId = IdPool::GetInstance().GetId(
        std::make_tuple(deviceId, taskData.streamId, taskData.taskId, taskData.contextId, taskData.batchId));
    taskData.taskType = IdPool::GetInstance().GetUint64Id(hcclData.HCCLName);
    taskData.groupName = GetGroupNameValue(hcclData.groupName, hashMap);
    taskData.rdmaType = GetEnumTypeValue(hcclData.rdmaType, MSG_STR(HCCL_RDMA_TYPE_TABLE), HCCL_RDMA_TYPE_TABLE);
    taskData.transportType = GetEnumTypeValue(hcclData.transportType,
                                              MSG_STR(HCCL_TRANSPORT_TYPE_TABLE), HCCL_TRANSPORT_TYPE_TABLE);
    taskData.dataType = GetEnumTypeValue(hcclData.dataType, MSG_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
    taskData.linkType = GetEnumTypeValue(hcclData.linkType, MSG_STR(HCCL_LINK_TYPE_TABLE), HCCL_LINK_TYPE_TABLE);
    taskData.opId = IdPool::GetInstance().GetUint32Id(Utils::Join("_", hcclData.opName, hcclData.groupName, deviceId));
}

void CommunicationInfoProcessor::UpdateOpInfo(CommunicationOpData &opData, uint32_t connectionId,
                                              const std::unordered_map<uint32_t, size_t> &opInfoIdxMap,
                                              const OriOpDataFormat &oriOpData, GeHashMap &hashMap)
{
    auto indexIt = opInfoIdxMap.find(connectionId);
    if (indexIt != opInfoIdxMap.end()) {
        const auto &oriData = oriOpData[indexIt->second];
        std::string opName;
        std::string dataType;
        std::string algType;
        std::string groupName;
        std::string opType;
        std::tie(connectionId, opName, opData.relay, opData.retry, dataType, algType,
                 opData.count, groupName, opType) = oriData;
        opData.dataType = GetEnumTypeValue(dataType, MSG_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
        opData.algType = IdPool::GetInstance().GetUint64Id(algType);
        opData.groupName = GetGroupNameValue(groupName, hashMap);
        opData.opType = IdPool::GetInstance().GetUint64Id(opType);
    }
}

std::unordered_map<uint32_t, size_t> CommunicationInfoProcessor::GenOpInfoIdxmap(const OriOpDataFormat &oriOpData)
{
    std::unordered_map<uint32_t, size_t> opInfoIdxMap;
    uint32_t connectionId = 0;
    for (size_t i = 0; i < oriOpData.size(); ++i) {
        connectionId = std::get<0>(oriOpData[i]);
        opInfoIdxMap[connectionId] = i;
    }
    return opInfoIdxMap;
}

bool CommunicationInfoProcessor::Process(const std::string &fileDir)
{
    ThreadData threadData;
    DBInfo taskDBInfo("hccl_single_device.db", "HCCLTaskSingleDevice");
    DBInfo opDBInfo("hccl_single_device.db", "HCCLOpSingleDevice");
    threadData.profId = IdPool::GetInstance().GetUint32Id(fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    GeHashMap hashMap;
    if (!GetGeHashMap(hashMap, fileDir)) {
        return false; // GetGeHashMap方法内有日志输出，这里直接返回
    }
    bool flag = true;
    for (const auto& devicePath: deviceList) {
        if (!ProcessOneDevice(devicePath, threadData, taskDBInfo, opDBInfo, fileDir, hashMap)) {
            flag = false;
        }
    }
    return flag;
}

bool CommunicationInfoProcessor::ProcessOneDevice(const std::string &devicePath, ThreadData &threadData,
                                                  DBInfo &taskDBInfo, DBInfo &opDBInfo,
                                                  const std::string &fileDir, GeHashMap &hashMap)
{
    std::string taskDBPath = Utils::File::PathJoin({devicePath, SQLITE, taskDBInfo.dbName});
    // 并不是所有场景都有hccl数据
    auto status = CheckPath(taskDBPath);
    if (status != CHECK_SUCCESS) {
        return status != CHECK_FAILED;
    }
    CommunicationTaskDataFormat taskData;
    CommunicationOpDataFormat opData;
    threadData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
    if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir)) {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    MAKE_SHARED_RETURN_VALUE(taskDBInfo.dbRunner, DBRunner, false, taskDBPath);
    auto oriTaskData = GetTaskData(taskDBInfo);
    if (oriTaskData.empty()) {
        ERROR("Get % data failed in %.", taskDBInfo.tableName, taskDBPath);
        return false;
    }
    std::string opDBPath = Utils::File::PathJoin({devicePath, SQLITE, opDBInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(opDBInfo.dbRunner, DBRunner, false, opDBPath);
    auto oriOpData = GetOpData(opDBInfo);
    if (!FormatData(oriTaskData, oriOpData, taskData, opData, threadData, hashMap)) {
        ERROR("Save % data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO, taskDBPath);
        return false;
    }
    if (!SaveData(taskData, TABLE_NAME_COMMUNICATION_TASK_INFO)) {
        ERROR("Save % data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO, taskDBPath);
        return false;
    }
    if (!CreateTableIndex(TABLE_NAME_COMMUNICATION_TASK_INFO, INDEX_NAME, COMMUNICATION_TASK_INDEX_COL_NAMES)) {
        ERROR("Create table index failed.");
        return false;
    }
    if (!SaveData(opData, TABLE_NAME_COMMUNICATION_OP)) {
        ERROR("Save % data failed, %.", TABLE_NAME_COMMUNICATION_OP, taskDBPath);
        return false;
    }
    return true;
}

}
}
}
