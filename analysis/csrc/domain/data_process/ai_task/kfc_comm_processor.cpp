/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "analysis/csrc/domain/data_process/ai_task/kfc_comm_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

KfcCommProcessor::KfcCommProcessor(const std::string &profPath) : CommunicationInfoProcessor(profPath) {}

bool KfcCommProcessor::Process(DataInventory& dataInventory)
{
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<KfcTaskData> resTask;
    std::vector<KfcOpData> resOp;
    CommunicationData communicationData;
    auto hashMap = dataInventory.GetPtr<GeHashMap>();
    if (hashMap == nullptr) {
        ERROR("Can't get hash data.");
        return false;
    }
    communicationData.hashMap = *hashMap;
    for (const auto& devicePath : deviceList) {
        flag = ProcessSingleDevice(communicationData, devicePath, resTask, resOp) && flag;
    }
    if (!SaveData(resTask, resOp, dataInventory)) {
        ERROR("Save kfc data failed, %.", PROCESSOR_NAME_KFC_COMM);
        flag = false;
    }
    return flag;
}

bool KfcCommProcessor::ProcessSingleDevice(CommunicationInfoProcessor::CommunicationData& communicationData,
                                           const std::string& devicePath, std::vector<KfcTaskData>& resTask,
                                           std::vector<KfcOpData>& resOp)
{
    communicationData.deviceId = GetDeviceIdByDevicePath(devicePath);
    if (!Context::GetInstance().GetProfTimeRecordInfo(communicationData.timeRecord, profPath_,
                                                      communicationData.deviceId)) {
        ERROR("Failed to obtain the time in start_info and end_info. Prof path is %, device id is %.", profPath_,
              communicationData.deviceId);
        return false;
    }
    DBInfo KfcOPTable("hccl_single_device.db", "KfcOP");
    DBInfo KfcTaskTable("hccl_single_device.db", "KfcTask");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, KfcOPTable.dbName});
    if (!KfcOPTable.ConstructDBRunner(dbPath) || !KfcTaskTable.ConstructDBRunner(dbPath)) {
        return false;
    }
    auto commStatus = CheckPathAndTable(dbPath, KfcOPTable, false);
    auto computeStatus = CheckPathAndTable(dbPath, KfcTaskTable, false);
    if (commStatus != CHECK_SUCCESS && computeStatus != CHECK_SUCCESS) {
        if (commStatus == CHECK_FAILED || computeStatus == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    // db取出所有的数据
    communicationData.oriTaskData = LoadTaskData(KfcTaskTable);
    communicationData.oriOpData = LoadOpData(KfcOPTable);
    std::vector<CommunicationTaskData> taskData;
    std::vector<CommunicationOpData> opData;
    if (!FormatData(taskData, opData, communicationData)) {
        ERROR("Format data failed, %.", TABLE_NAME_COMMUNICATION_TASK_INFO);
        return false;
    }
    FilterDataByStartTime(taskData, communicationData.timeRecord.startTimeNs, TABLE_NAME_COMMUNICATION_TASK_INFO);
    FilterDataByStartTime(opData, communicationData.timeRecord.startTimeNs, TABLE_NAME_COMMUNICATION_OP);
    return ConvertTaskData(resTask, taskData) & ConvertOpData(resOp, opData);
}

bool KfcCommProcessor::SaveData(std::vector<KfcTaskData> &resTask, std::vector<KfcOpData> &resOp,
                                DataInventory &dataInventory)
{
    if (!SaveToDataInventory<KfcTaskData>(std::move(resTask), dataInventory, PROCESSOR_NAME_KFC_COMM)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_KFC_COMM);
        return false;
    }
    if (!SaveToDataInventory<KfcOpData>(std::move(resOp), dataInventory, PROCESSOR_NAME_KFC_COMM)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_KFC_COMM);
        return false;
    }
    return true;
}

OriOpDataFormat KfcCommProcessor::LoadOpData(const DBInfo &kfcDB)
{
    OriOpDataFormat oriOpData;
    std::string sql{"SELECT connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type, "
                    "model_id, rank_size FROM " + kfcDB.tableName + " WHERE source != "
                    + std::to_string(static_cast<int>(DeviceHcclOpSource::HCCL))};
    if (!kfcDB.dbRunner->QueryData(sql, oriOpData)) {
        ERROR("Failed to obtain data from the % table.", kfcDB.tableName);
    }
    return oriOpData;
}

OriTaskDataFormat KfcCommProcessor::LoadTaskData(const DBInfo& taskSingleDevice)
{
    OriTaskDataFormat oriTaskData;
    std::string sql{"SELECT model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
                    "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
                    "rdma_type, timestamp, duration, connection_id, duration_estimated, bandwidth, is_master "
                    "FROM " + taskSingleDevice.tableName + " WHERE source != "
                    + std::to_string(static_cast<int>(DeviceHcclOpSource::HCCL))};
    if (!taskSingleDevice.dbRunner->QueryData(sql, oriTaskData)) {
        ERROR("Failed to obtain data from the % table.", taskSingleDevice.tableName);
    }
    return oriTaskData;
}

bool KfcCommProcessor::ConvertTaskData(std::vector<KfcTaskData> &resTask,
                                       const std::vector<CommunicationTaskData> &taskData)
{
    if (!Reserve(resTask, taskData.size())) {
        ERROR("Reserve for kfc task data failed.");
        return false;
    }
    for (const auto &data : taskData) {
        auto groupName = data.groupName + " Aicpu";
        resTask.emplace_back(data.deviceId, data.modelId, data.taskType, data.timestamp, data.duration, data.notifyId,
                             data.durationEstimated, data.streamId, data.taskId, data.contextId, data.batchId,
                             data.taskType, data.srcRank, data.dstRank, data.transportType, data.size, data.linkType,
                             data.bandwidth, groupName, data.planeId, data.dataType, data.isMaster, data.opName,
                             data.opKey, data.rdmaType);
    }
    return true;
}

bool KfcCommProcessor::ConvertOpData(std::vector<KfcOpData> &resOp, const std::vector<CommunicationOpData> &opData)
{
    if (!Reserve(resOp, opData.size())) {
        ERROR("Reserve for kfc task data failed.");
        return false;
    }
    for (const auto &data : opData) {
        auto groupName = data.groupName + " Aicpu";
        resOp.emplace_back(data.deviceId, data.opName, data.timestamp, data.end, groupName, data.connectionId,
                           data.modelId, data.dataType, data.count, data.algType, data.rankSize, data.relay,
                           data.retry, data.opType, data.opKey);
    }
    return true;
}
}
}