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
const std::string INDEX_NAME = "CommunicationTaskIndex";
const std::vector<std::string> COMMUNICATION_TASK_INDEX_COL_NAMES = {"globalTaskId"};
struct CommunicationOpEndpointsTime {
    double firstTaskStartTime;
    double lastTaskStartTime;
    double lastTaskDuration;
};

// groupName 依据hash进行转换，对于无hash的数据，直接取用hash值（即groupName）进行转换
uint64_t GetGroupNameValue(const std::string &groupName, GeHashMap &hashMap)
{
    if (groupName != NA && Utils::IsNumber(groupName)) {
        if (hashMap.find(groupName) != hashMap.end()) {
            return IdPool::GetInstance().GetUint64Id(hashMap[groupName]);
        }
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

CommunicationInfoProcessor::OriTaskDataFormat CommunicationInfoProcessor::GetKfcTaskData(const DBInfo &kfcTask)
{
    OriTaskDataFormat oriTaskData;
    std::string sql{"SELECT model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
                    "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
                    "rdma_type, timestamp, duration, -1 as connection_id FROM " + kfcTask.tableName};
    if (!kfcTask.dbRunner->QueryData(sql, oriTaskData)) {
        ERROR("Failed to obtain data from the % table.", kfcTask.tableName);
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

CommunicationInfoProcessor::OriKfcOpDataFormat CommunicationInfoProcessor::GetKfcOpData(const DBInfo &kfcOp)
{
    OriKfcOpDataFormat oriOpData;
    std::string sql{"SELECT connection_id, op_name, -1 as relay, -1 as retry, 'N/A' as data_type, "
                    "'N/A' as alg_type, " + std::to_string(UINT64_MAX) + " as count, group_name, "
                    "op_type, timestamp, duration FROM " + kfcOp.tableName};
    if (!kfcOp.dbRunner->QueryData(sql, oriOpData)) {
        ERROR("Failed to obtain data from the % table.", kfcOp.tableName);
    }
    return oriOpData;
}

void CommunicationInfoProcessor::FormatData(const OriTaskDataFormat &oriTaskData, const OriOpDataFormat &oriOpData,
                                            CommunicationTaskDataFormat &processedTaskData,
                                            CommunicationOpDataFormat &processedOpData,
                                            const ThreadData &threadData, GeHashMap &hashMap)
{
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
}

void CommunicationInfoProcessor::FormatKfcTaskData(const OriTaskDataFormat &oriTaskData,
                                                   CommunicationTaskDataFormat &processedTaskData,
                                                   const ThreadData &threadData, GeHashMap &hashMap)
{
    CommunicationTaskData taskData;
    HcclTaskSingleDeviceData hcclData;
    for (auto &row: oriTaskData) {
        Update(row, hcclData, taskData, threadData.deviceId, hashMap);
        processedTaskData.emplace_back(taskData.opName, taskData.globalTaskId, taskData.taskType, taskData.planeId,
                                       taskData.groupName, taskData.notifyId, taskData.rdmaType, taskData.srcRank,
                                       taskData.dstRank, taskData.transportType, taskData.size, taskData.dataType,
                                       taskData.linkType, taskData.opId);
    }
}

void CommunicationInfoProcessor::FormatKfcOpData(const OriKfcOpDataFormat &oriOpData,
                                                 CommunicationOpDataFormat &processedOpData,
                                                 const ThreadData &threadData, GeHashMap &hashMap)
{
    uint32_t connectionId;
    std::string opName;
    int32_t relay = 0;
    int32_t retry = 0;
    std::string dataType;
    std::string algType;
    uint64_t count = UINT64_MAX;
    std::string groupName;
    std::string opType;
    double timestamp;
    double duration;
    for (auto &data: oriOpData) {
        std::tie(connectionId, opName, relay, retry, dataType, algType,
                 count, groupName, opType, timestamp, duration) = data;
        HPFloat start{timestamp};
        HPFloat end = start + HPFloat(duration);
        auto startTime = Utils::GetLocalTime(start, threadData.timeRecord).Uint64();
        auto endTime = Utils::GetLocalTime(end, threadData.timeRecord).Uint64();
        auto opId = IdPool::GetInstance().GetUint32Id(Utils::Join("_", opName, groupName, threadData.deviceId));
        auto opNameId = IdPool::GetInstance().GetUint64Id(opName);
        auto groupNameValue = GetGroupNameValue(groupName, hashMap);
        auto dataTypeValue = GetEnumTypeValue(dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
        auto algTypeValue = IdPool::GetInstance().GetUint64Id(algType);
        auto opTypeValue = IdPool::GetInstance().GetUint64Id(opType);
        processedOpData.emplace_back(opNameId, startTime, endTime,
                                     Utils::Contact(threadData.profId, connectionId), groupNameValue, opId,
                                     relay, retry, dataTypeValue, algTypeValue, count, opTypeValue);
    }
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
    taskData.rdmaType = GetEnumTypeValue(hcclData.rdmaType, NAME_STR(HCCL_RDMA_TYPE_TABLE), HCCL_RDMA_TYPE_TABLE);
    taskData.transportType = GetEnumTypeValue(hcclData.transportType,
                                              NAME_STR(HCCL_TRANSPORT_TYPE_TABLE), HCCL_TRANSPORT_TYPE_TABLE);
    taskData.dataType = GetEnumTypeValue(hcclData.dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
    taskData.linkType = GetEnumTypeValue(hcclData.linkType, NAME_STR(HCCL_LINK_TYPE_TABLE), HCCL_LINK_TYPE_TABLE);
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
        opData.dataType = GetEnumTypeValue(dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
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
    DBInfo kfcTaskDBInfo("hccl_single_device.db", "KfcTask");
    DBInfo kfcOpDBInfo("hccl_single_device.db", "KfcOP");
    threadData.profId = IdPool::GetInstance().GetUint32Id(fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool flag = true;
    for (const auto& devicePath: deviceList) {
        OriTaskDataFormat oriTaskData;
        OriOpDataFormat oriOpData;
        if (!ProcessOneDeviceHccl(devicePath, taskDBInfo, opDBInfo, oriTaskData, oriOpData)) {
            ERROR("ProcessOneDeviceHccl failed.");
            flag = false;
        }
        OriTaskDataFormat oriKfcTaskData;
        OriKfcOpDataFormat oriKfcOpData;
        if (!ProcessOneDeviceKfc(devicePath, kfcTaskDBInfo, kfcOpDBInfo, oriKfcTaskData, oriKfcOpData)) {
            ERROR("ProcessOneDeviceKfc failed.");
            flag = false;
        }
        threadData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (!SaveCommData(threadData, oriTaskData, oriOpData, oriKfcTaskData, oriKfcOpData, fileDir)) {
            ERROR("Save communication data failed.");
            flag = false;
        }
    }
    return flag;
}

bool CommunicationInfoProcessor::ProcessOneDeviceHccl(const std::string &devicePath, DBInfo &taskDBInfo,
                                                      DBInfo &opDBInfo, OriTaskDataFormat &oriTaskData,
                                                      OriOpDataFormat &oriOpData)
{
    std::string taskDBPath = Utils::File::PathJoin({devicePath, SQLITE, taskDBInfo.dbName});
    std::string opDBPath = Utils::File::PathJoin({devicePath, SQLITE, opDBInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(taskDBInfo.dbRunner, DBRunner, false, taskDBPath);
    MAKE_SHARED_RETURN_VALUE(opDBInfo.dbRunner, DBRunner, false, opDBPath);
    // 并不是所有场景都有hccl数据
    auto status = CheckPathAndTable(taskDBPath, taskDBInfo);
    if (status != CHECK_SUCCESS) {
        INFO("Check % data exit in %.", taskDBInfo.tableName, taskDBPath);
        return status != CHECK_FAILED;
    }
    status = CheckPathAndTable(opDBPath, opDBInfo);
    if (status != CHECK_SUCCESS) {
        INFO("Check % data exit in %.", opDBInfo.tableName, opDBPath);
        return status != CHECK_FAILED;
    }
    oriTaskData = GetTaskData(taskDBInfo);
    if (oriTaskData.empty()) {
        ERROR("Get % data failed in %.", taskDBInfo.tableName, taskDBPath);
        return false;
    }
    oriOpData = GetOpData(opDBInfo);
    return true;
}

bool CommunicationInfoProcessor::ProcessOneDeviceKfc(const std::string &devicePath, DBInfo &kfcTaskDBInfo,
                                                     DBInfo &kfcOpDBInfo, OriTaskDataFormat &oriKfcTaskData,
                                                     OriKfcOpDataFormat &oriKfcOpData)
{
    std::string kfcTaskDBPath = Utils::File::PathJoin({devicePath, SQLITE, kfcTaskDBInfo.dbName});
    std::string kfcOpDBPath = Utils::File::PathJoin({devicePath, SQLITE, kfcOpDBInfo.dbName});
    MAKE_SHARED_RETURN_VALUE(kfcTaskDBInfo.dbRunner, DBRunner, false, kfcOpDBPath);
    MAKE_SHARED_RETURN_VALUE(kfcOpDBInfo.dbRunner, DBRunner, false, kfcOpDBPath);
    auto status = CheckPathAndTable(kfcTaskDBPath, kfcTaskDBInfo);
    if (status == CHECK_SUCCESS) {
        oriKfcTaskData = GetKfcTaskData(kfcTaskDBInfo);
    } else if (status == CHECK_FAILED) {
        ERROR("Check % data failed in %.", kfcTaskDBInfo.tableName, kfcTaskDBPath);
        return false;
    } else {
        INFO("Not exist KfcTask table");
    }
    status = CheckPathAndTable(kfcOpDBPath, kfcOpDBInfo);
    if (status == CHECK_SUCCESS) {
        oriKfcOpData = GetKfcOpData(kfcOpDBInfo);
    } else if (status == CHECK_FAILED) {
        ERROR("Check % data failed in %.", kfcOpDBInfo.tableName, kfcOpDBPath);
        return false;
    } else {
        INFO("Not exist KfcOP table");
    }
    return true;
}

bool CommunicationInfoProcessor::SaveCommData(ThreadData &threadData, OriTaskDataFormat &oriTaskData,
                                              OriOpDataFormat &oriOpData, OriTaskDataFormat &oriKfcTaskData,
                                              OriKfcOpDataFormat &oriKfcOpData, const std::string &fileDir)
{
    if (oriTaskData.empty() && oriOpData.empty() && oriKfcTaskData.empty() && oriKfcOpData.empty()) {
        INFO("No communication data");
        return true;
    }
    GeHashMap hashMap;
    if (!GetGeHashMap(hashMap, fileDir)) {
        ERROR("Can't get hash data.");
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir)) {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    CommunicationTaskDataFormat taskData;
    CommunicationOpDataFormat opData;
    if (!Utils::Reserve(taskData, oriTaskData.size() + oriKfcTaskData.size())) {
        ERROR("Reserve for communication task data failed.");
        return false;
    }
    if (!Utils::Reserve(opData, oriOpData.size() + oriKfcOpData.size())) {
        ERROR("Reserve for communication op data failed.");
        return false;
    }
    FormatData(oriTaskData, oriOpData, taskData, opData, threadData, hashMap);
    FormatKfcTaskData(oriKfcTaskData, taskData, threadData, hashMap);
    FormatKfcOpData(oriKfcOpData, opData, threadData, hashMap);
    if (!taskData.empty() && !SaveData(taskData, TABLE_NAME_COMMUNICATION_TASK_INFO)) {
        ERROR("Save % data failed.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        return false;
    }
    if (!taskData.empty() &&
        !CreateTableIndex(TABLE_NAME_COMMUNICATION_TASK_INFO, INDEX_NAME, COMMUNICATION_TASK_INDEX_COL_NAMES)) {
        ERROR("Create table index failed.");
        return false;
    }
    if (!opData.empty() && !SaveData(opData, TABLE_NAME_COMMUNICATION_OP)) {
        ERROR("Save % data failed.", TABLE_NAME_COMMUNICATION_OP);
        return false;
    }
    return true;
}
}
}
}
