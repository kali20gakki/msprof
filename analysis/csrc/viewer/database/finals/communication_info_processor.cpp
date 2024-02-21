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

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;
using namespace Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct CommunicationOpData {
    uint64_t opName = UINT64_MAX;
    uint64_t groupName = UINT64_MAX;
    uint64_t connectionId = UINT64_MAX;
    uint32_t opId = UINT32_MAX;
    std::string start;
    std::string end;
};
struct CommunicationOpEndpointsTime {
    double firstTaskStartTime;
    double lastTaskStartTime;
    double lastTaskDuration;
};
}

CommunicationInfoProcessor::CommunicationInfoProcessor(const std::string &reportDBPath,
                                                       const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool CommunicationInfoProcessor::Run()
{
    INFO("CommunicationInfoProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_COMMUNICATION);
    return flag;
}

CommunicationInfoProcessor::OriDataFormat CommunicationInfoProcessor::GetData(const DBInfo &hcclSingleDeviceDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
                    "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
                    "rdma_type, timestamp, duration, connection_id FROM " + hcclSingleDeviceDB.tableName};
    if (!hcclSingleDeviceDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", hcclSingleDeviceDB.tableName);
    }
    return oriData;
}

bool CommunicationInfoProcessor::FormatData(const OriDataFormat &oriData,
                                            CommunicationTaskDataFormat &processedTaskData,
                                            CommunicationOpDataFormat &processedOpData, const ThreadData &threadData)
{
    CommunicationTaskData taskData;
    HcclSingleDeviceData hcclData;
    std::unordered_map<uint32_t, CommunicationOpData> opData;
    std::unordered_map<uint32_t, CommunicationOpEndpointsTime> endpoints;
    if (!Utils::Reserve(processedTaskData, oriData.size())) {
        ERROR("Reserve for communication task data failed.");
        return false;
    }
    for (auto &row: oriData) {
        Update(row, hcclData, taskData, threadData.deviceId);
        processedTaskData.emplace_back(taskData.opName, taskData.correlationId, taskData.taskType, taskData.planeId,
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
        data.start = Utils::GetLocalTime(start, threadData.timeRecord).Str();
        data.end = Utils::GetLocalTime(end, threadData.timeRecord).Str();
        processedOpData.emplace_back(data.opName, data.start, data.end, data.connectionId, data.groupName, data.opId);
    }
    return true;
}

void CommunicationInfoProcessor::Update(const HcclFormat &oriData, HcclSingleDeviceData &hcclData,
                                        CommunicationTaskData &taskData, uint16_t deviceId)
{
    std::tie(taskData.modelId, hcclData.opName, hcclData.HCCLName, hcclData.groupName, taskData.planeId,
             taskData.streamId, taskData.taskId, taskData.srcRank, taskData.dstRank,
             hcclData.transportType, taskData.size, hcclData.dataType, hcclData.linkType,
             taskData.contextId, taskData.notifyId, taskData.batchId, hcclData.rdmaType,
             hcclData.timestamp, hcclData.duration, hcclData.connectionId) = oriData;
    taskData.opName = IdPool::GetInstance().GetUint64Id(hcclData.opName);
    taskData.correlationId = IdPool::GetInstance().GetId(
        std::make_tuple(deviceId, taskData.modelId, taskData.streamId, taskData.taskId, taskData.contextId,
                        taskData.batchId));
    taskData.taskType = IdPool::GetInstance().GetUint64Id(hcclData.HCCLName);
    taskData.groupName = IdPool::GetInstance().GetUint64Id(hcclData.groupName);
    taskData.rdmaType = IdPool::GetInstance().GetUint64Id(hcclData.rdmaType);
    taskData.transportType = IdPool::GetInstance().GetUint64Id(hcclData.transportType);
    taskData.dataType = IdPool::GetInstance().GetUint64Id(hcclData.dataType);
    taskData.linkType = IdPool::GetInstance().GetUint64Id(hcclData.linkType);
    taskData.opId = IdPool::GetInstance().GetUint32Id(Utils::Join("_", hcclData.opName, hcclData.groupName, deviceId));
}

bool CommunicationInfoProcessor::Process(const std::string &fileDir)
{
    bool flag = true;
    ThreadData threadData;
    threadData.profId = IdPool::GetInstance().GetUint32Id(fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, threadData.hcclSingleDeviceDB.dbName});
        // 并不是所有场景都有hccl数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        CommunicationTaskDataFormat taskData;
        CommunicationOpDataFormat opData;
        threadData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir)) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            flag = false;
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(threadData.hcclSingleDeviceDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(threadData.hcclSingleDeviceDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", threadData.hcclSingleDeviceDB.tableName, dbPath);
            continue;
        }
        if (!FormatData(oriData, taskData, opData, threadData)) {
            flag = false;
            ERROR("Save % data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO, dbPath);
            continue;
        }
        if (!SaveData(taskData, TABLE_NAME_COMMUNICATION_TASK_INFO)) {
            flag = false;
            ERROR("Save % data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO, dbPath);
            continue;
        }
        if (!SaveData(opData, TABLE_NAME_COMMUNICATION_OP)) {
            flag = false;
            ERROR("Save % data failed, %.", TABLE_NAME_COMMUNICATION_OP, dbPath);
            continue;
        }
    }
    return flag;
}

}
}
}