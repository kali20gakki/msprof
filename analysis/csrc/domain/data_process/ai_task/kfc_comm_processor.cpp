/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kfc_comm_processor.cpp
 * Description        : kfc通信数据导出流程
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/kfc_comm_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/services/constant/time_unit_constant.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
std::string GetGroupNameValue(const std::string& groupName, std::unordered_map<std::string, std::string>& hashMap)
{
    if (groupName != NA && Utils::IsNumber(groupName)) {
        if (hashMap.find(groupName) != hashMap.end()) {
            return hashMap[groupName];
        }
    }
    return groupName;
}
}

KfcCommProcessor::KfcCommProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool KfcCommProcessor::Process(DataInventory& dataInventory)
{
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<KfcTaskData> resTask;
    std::vector<KfcOpData> resOp;
    auto hashMap = dataInventory.GetPtr<std::unordered_map<std::string, std::string>>();
    if (hashMap == nullptr) {
        ERROR("Can't get hash data.");
        return false;
    }
    for (const auto& devicePath : deviceList) {
        DBInfo KfcOPTable("hccl_single_device.db", "KfcOP");
        DBInfo KfcTaksTable("hccl_single_device.db", "KfcTask");
        std::string dbPath = File::PathJoin({devicePath, SQLITE, KfcOPTable.dbName});
        if (!KfcOPTable.ConstructDBRunner(dbPath) || !KfcTaksTable.ConstructDBRunner(dbPath)) {
            flag = false;
            continue;
        }

        auto commStatus = CheckPathAndTable(dbPath, KfcOPTable, false);
        auto computeStatus = CheckPathAndTable(dbPath, KfcTaksTable, false);
        if (commStatus != CHECK_SUCCESS && computeStatus != CHECK_SUCCESS) {
            if (commStatus == CHECK_FAILED || computeStatus == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }

        // db取出所有的数据
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        auto taskData = LoadTaskData(KfcTaksTable, dbPath);
        auto opData = LoadOPData(KfcOPTable, dbPath);
        auto formatKfcTask = FormatTaskData(taskData, *hashMap, deviceId);
        auto formatOpData = FormatOPData(opData, *hashMap, deviceId);
        resOp.insert(resOp.end(), formatOpData.begin(), formatOpData.end());
        resTask.insert(resTask.end(), formatKfcTask.begin(), formatKfcTask.end());
    }
    if (!SaveToDataInventory<KfcTaskData>(std::move(resTask), dataInventory, PROCESSOR_NAME_KFC_COMM)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_KFC_COMM);
        flag = false;
    }

    if (!SaveToDataInventory<KfcOpData>(std::move(resOp), dataInventory, PROCESSOR_NAME_KFC_COMM)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_KFC_COMM);
        flag = false;
    }
    return flag;
}

std::vector<KfcTaskDto> KfcCommProcessor::LoadTaskData(const DBInfo &kfcDB, const std::string &dbPath)
{
    std::vector<KfcTaskDto> result;
    OriKfcTaskData oriTaskData;
    std::string sql{"SELECT model_id, index_id, op_name, op_timestamp, op_duration, iteration, hccl_name, group_name, "
                    "plane_id, timestamp, duration, stream_id, task_id, duration_estimated, local_rank, remote_rank, "
                    "transport_type, size, data_type, link_type, bandwidth, context_id, notify_id, batch_id, rdma_type "
                    "FROM " + kfcDB.tableName};
    if (!kfcDB.dbRunner->QueryData(sql, oriTaskData)) {
        ERROR("Failed to obtain data from the % table.", kfcDB.tableName);
    }

    for (const auto &row: oriTaskData) {
        KfcTaskDto kfcTask{};
        std::tie(kfcTask.modelId, kfcTask.indexId, kfcTask.opName, kfcTask.opTimestamp, kfcTask.opDuration,
                 kfcTask.iteration, kfcTask.hcclName, kfcTask.groupName, kfcTask.planeId, kfcTask.timestamp,
                 kfcTask.duration, kfcTask.streamId, kfcTask.taskId, kfcTask.durationEstimated, kfcTask.localRank,
                 kfcTask.remoteRank, kfcTask.transportType, kfcTask.size, kfcTask.dataType, kfcTask.linkType,
                 kfcTask.bandwidth, kfcTask.contextId, kfcTask.notifyId, kfcTask.batchId, kfcTask.rdmaType) = row;
        result.push_back(kfcTask);
    }

    return result;
}

std::vector<KfcOPDto> KfcCommProcessor::LoadOPData(const DBInfo &kfcDB, const std::string &dbPath)
{
    OriKfcOPData oriOpData;
    std::vector<KfcOPDto> result;
    std::string sql{"SELECT model_id, index_id, op_name, timestamp, duration, group_name, connection_id, data_type, "
                    "alg_type, count, rank_size FROM " + kfcDB.tableName};
    if (!kfcDB.dbRunner->QueryData(sql, oriOpData)) {
        ERROR("Failed to obtain data from the % table.", kfcDB.tableName);
    }
    for (const auto &row: oriOpData) {
        KfcOPDto kfcOp{};
        std::tie(kfcOp.modelId, kfcOp.indexId, kfcOp.opName, kfcOp.timestamp, kfcOp.duration, kfcOp.groupName,
                 kfcOp.connectionId, kfcOp.dataType, kfcOp.algType, kfcOp.count, kfcOp.rankSize) = row;
        result.push_back(kfcOp);
    }

    return result;
}

std::vector<KfcTaskData> KfcCommProcessor::FormatTaskData(const std::vector<KfcTaskDto> &oriCommData,
    std::unordered_map<std::string, std::string>& hashMap, uint16_t deviceId)
{
    ProfTimeRecord profTimeRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(profTimeRecord, profPath_, deviceId)) {
        ERROR("Failed to obtain the time in start_info and end_info. "
              "Prof path is %, device id is %.", profPath_, deviceId);
        return {};
    }
    std::vector<KfcTaskData> result;
    uint64_t transportType;
    uint64_t dataType;
    uint64_t linkType;
    std::string groupName;
    for (const auto &oriData: oriCommData) {
        HPFloat timeStamp = {oriData.timestamp};
        transportType = GetEnumTypeValue(oriData.transportType, NAME_STR(HCCL_TRANSPORT_TYPE_TABLE),
                                         HCCL_TRANSPORT_TYPE_TABLE);
        dataType = GetEnumTypeValue(oriData.dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
        linkType = GetEnumTypeValue(oriData.linkType, NAME_STR(HCCL_LINK_TYPE_TABLE), HCCL_LINK_TYPE_TABLE);
        groupName = GetGroupNameValue(oriData.groupName, hashMap);
        result.emplace_back(deviceId, oriData.modelId, oriData.hcclName,
                            GetLocalTime(timeStamp, profTimeRecord).Uint64(),
                            static_cast<double>(oriData.duration), oriData.notifyId,
                            static_cast<double>(oriData.durationEstimated), oriData.streamId, oriData.taskId,
                            oriData.contextId, " ", oriData.localRank, oriData.remoteRank, transportType,
                            oriData.size, linkType, oriData.bandwidth, groupName, oriData.planeId, dataType);
    }
    return result;
}

std::vector<KfcOpData> KfcCommProcessor::FormatOPData(const std::vector<KfcOPDto> &oriOpData,
    std::unordered_map<std::string, std::string>& hashMap,  uint16_t deviceId)
{
    ProfTimeRecord profTimeRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(profTimeRecord, profPath_, deviceId)) {
        ERROR("Failed to obtain the time in start_info and end_info. "
              "Prof path is %, device id is %.", profPath_, deviceId);
        return {};
    }
    std::vector<KfcOpData> result;
    uint64_t dataType;
    std::string groupName;
    uint64_t start;
    for (const auto &opData : oriOpData) {
        dataType = GetEnumTypeValue(opData.dataType, NAME_STR(HCCL_DATA_TYPE_TABLE), HCCL_DATA_TYPE_TABLE);
        groupName = GetGroupNameValue(opData.groupName, hashMap);
        HPFloat timeStamp = {opData.timestamp};
        start = GetLocalTime(timeStamp, profTimeRecord).Uint64();
        // start_index需要
        result.emplace_back(deviceId, opData.opName, start, start + opData.duration, groupName, opData.connectionId,
                            opData.modelId, dataType, opData.count, opData.algType, opData.rankSize);
    }
    return result;
}
}
}