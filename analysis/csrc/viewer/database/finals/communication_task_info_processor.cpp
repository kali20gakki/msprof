/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_task_info_processor.cpp
 * Description        : communication_task_info_processor，处理表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/communication_task_info_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/association/credential/id_pool.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;
namespace {
struct CommunicationTaskData {
    int32_t planeId = UINT32_MAX;
    uint32_t modelId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    uint32_t contextId = UINT32_MAX;
    uint32_t batchId = UINT32_MAX;
    uint32_t srcRank = UINT32_MAX;
    uint32_t dstRank = UINT32_MAX;
    uint64_t correlationId = UINT64_MAX;
    uint64_t opName = UINT64_MAX;
    uint64_t taskType = UINT64_MAX;
    uint64_t groupName = UINT64_MAX;
    uint64_t transportType = UINT64_MAX;
    uint64_t size = UINT64_MAX;
    uint64_t dataType = UINT64_MAX;
    uint64_t linkType = UINT64_MAX;
    uint64_t notifyId = UINT64_MAX;
    uint64_t rdmaType = UINT64_MAX;
};
}

CommunicationTaskInfoProcessor::CommunicationTaskInfoProcessor(const std::string &reportDBPath,
                                                               const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool CommunicationTaskInfoProcessor::Run()
{
    INFO("EnumApiLevelProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, TABLE_NAME_COMMUNICATION_TASK_INFO);
    return flag;
}

CommunicationTaskInfoProcessor::OriDataFormat CommunicationTaskInfoProcessor::GetData(DBInfo &hcclSingleDeviceDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, "
                    "remote_rank, transport_type, size, data_type, link_type, context_id, notify_id, batch_id, "
                    "rdma_type FROM " + hcclSingleDeviceDB.tableName};
    if (!hcclSingleDeviceDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", hcclSingleDeviceDB.tableName);
    }
    return oriData;
}

CommunicationTaskInfoProcessor::ProcessedDataFormat CommunicationTaskInfoProcessor::FormatData(
    const OriDataFormat &oriData, uint16_t deviceId)
{
    ProcessedDataFormat processedData;
    CommunicationTaskData data;
    std::string oriOpName;
    std::string oriHCCLName;
    std::string oriGroupName;
    std::string oriTransportType;
    std::string oriDataType;
    std::string oriLinkType;
    std::string oriRdmaType;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for communication task data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.modelId, oriOpName, oriHCCLName, oriGroupName, data.planeId, data.streamId,
                 data.taskId, data.srcRank, data.dstRank, oriTransportType, data.size, oriDataType,
                 oriLinkType, data.contextId, data.notifyId, data.batchId, oriRdmaType) = row;
        data.opName = IdPool::GetInstance().GetUint64Id(oriOpName);
        data.correlationId = IdPool::GetInstance().GetId(
            std::make_tuple(deviceId, data.modelId, data.streamId, data.taskId, data.contextId, data.batchId));
        data.taskType = IdPool::GetInstance().GetUint64Id(oriHCCLName);
        data.groupName = IdPool::GetInstance().GetUint64Id(oriGroupName);
        data.rdmaType = IdPool::GetInstance().GetUint64Id(oriRdmaType);
        data.transportType = IdPool::GetInstance().GetUint64Id(oriTransportType);
        data.dataType = IdPool::GetInstance().GetUint64Id(oriDataType);
        data.linkType = IdPool::GetInstance().GetUint64Id(oriLinkType);
        processedData.emplace_back(data.opName, data.correlationId, data.taskType, data.planeId, data.groupName,
                                   data.notifyId, data.rdmaType, data.srcRank, data.dstRank, data.transportType,
                                   data.size, data.dataType, data.linkType);
    }
    return processedData;
}

bool CommunicationTaskInfoProcessor::Process(const std::string &fileDir)
{
    bool flag = true;
    DBInfo hcclSingleDeviceDB("hccl_single_device.db", "HCCLSingleDevice");
    MAKE_SHARED0_RETURN_VALUE(hcclSingleDeviceDB.database, HCCLSingleDeviceDB, false);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        uint16_t deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, hcclSingleDeviceDB.dbName});
        // 并不是所有场景都有hccl数据
        if (!Utils::File::Exist(dbPath)) {
            WARN("Can't find the db, the path is %.", dbPath);
            continue;
        }
        if (!Utils::FileReader::Check(dbPath, MAX_DB_BYTES)) {
            flag = false;
            ERROR("Check % failed.", dbPath);
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(hcclSingleDeviceDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(hcclSingleDeviceDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", hcclSingleDeviceDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, deviceId);
        if (!SaveData(processedData, TABLE_NAME_COMMUNICATION_TASK_INFO)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
    }
    return flag;
}

}
}
}