/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_hccl_persistence.cpp
 * Description        : hccl single device数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/6/3
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/device_hccl_persistence.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/domain/services/association/calculator/hccl/include/hccl_calculator.h"
#include "analysis/csrc/domain/services/persistence/persistence_utils.h"

namespace Analysis {
namespace Domain {
using namespace Viewer::Database;
namespace {
// modelId, opName, taskType, opType, timestamp, relay, retry, dataType, algType, count, groupName, connectionId
using HcclOpDataFormat = std::vector<std::tuple<uint64_t, std::string, std::string, std::string, uint64_t, int32_t,
        int32_t, std::string, std::string, int32_t, std::string, int64_t>>;

// modelId, indexId, opName, iteration, hcclName, groupName, firstTimestamp, planeId, timestamp, duration, isDynamic,
// taskType, opType, connectionId, isMaster, streamId, taskId, durationEstimated, localRank, remoteRank, transportType,
// size, dataType, linkType, bandwidth, contextId, notifyId, batchId, rdmaType
using HcclTaskDataFormat = std::vector<std::tuple<uint64_t, int32_t, std::string, uint16_t, std::string, std::string,
        uint64_t, int32_t, uint64_t, uint64_t, std::string, std::string, std::string, int64_t, uint16_t, uint32_t,
        uint16_t, double, uint32_t, uint32_t, std::string, double, std::string, std::string, double, uint32_t,
        std::string, uint16_t, std::string>>;

// opType, count, totalTime, min, avg, max, ratio
using HcclStasticsFormat = std::vector<std::tuple<std::string, uint32_t, uint64_t, uint64_t, double, uint64_t, double>>;
}

bool SaveHcclOpData(DataInventory& dataInventory, const std::string devicePath)
{
    auto opData = dataInventory.GetPtr<std::vector<HcclOp>>();
    DBInfo hcclDB("hccl_single_device.db", "HCCLOpSingleDevice");
    MAKE_SHARED0_RETURN_VALUE(hcclDB.database, HCCLSingleDeviceDB, false);
    std::string dbPath = Utils::GetDBPath({devicePath, SQLITE, hcclDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(hcclDB.dbRunner, DBRunner, false, dbPath);
    HcclOpDataFormat saveData;
    if (!Utils::Reserve(saveData, opData->size())) {
        ERROR("Reserve for % data failed.", hcclDB.tableName);
        return false;
    }
    for (const auto& data : *opData) {
        saveData.emplace_back(data.modelId, data.opName, data.taskType, data.opType, data.timestamp, data.relay,
                              data.retry, data.dataType, data.algType, data.count, data.groupName, data.connectionId);
    }
    return SaveData(saveData, hcclDB, dbPath);
}

bool SaveHcclTaskData(DataInventory& dataInventory, const std::string devicePath)
{
    auto taskData = dataInventory.GetPtr<std::vector<DeviceHcclTask>>();
    DBInfo hcclDB("hccl_single_device.db", "HCCLTaskSingleDevice");
    MAKE_SHARED0_RETURN_VALUE(hcclDB.database, HCCLSingleDeviceDB, false);
    std::string dbPath = Utils::GetDBPath({devicePath, SQLITE, hcclDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(hcclDB.dbRunner, DBRunner, false, dbPath);
    HcclTaskDataFormat saveData;
    if (!Utils::Reserve(saveData, taskData->size())) {
        ERROR("Reserve for % data failed.", hcclDB.tableName);
        return false;
    }
    for (const auto& data : *taskData) {
        saveData.emplace_back(data.modelId, data.indexId, data.opName, data.iteration, data.hcclName, data.groupName,
                              data.firstTimestamp, data.planeId, data.timestamp, data.duration, data.isDynamic,
                              data.taskType, data.opType, data.connectionId, data.isMaster, data.streamId, data.taskId,
                              data.durationEstimated, data.localRank, data.remoteRank, data.transportType, data.size,
                              data.dataType, data.linkType, data.bandwidth, data.contextId, data.notifyId,
                              data.batchId, data.rdmaType);
    }
    return SaveData(saveData, hcclDB, dbPath);
}

bool SaveHcclStasticsData(DataInventory& dataInventory, const std::string devicePath)
{
    auto stasticsData = dataInventory.GetPtr<std::vector<HcclStastics>>();
    DBInfo hcclDB("hccl_single_device.db", "HcclOpReport");
    MAKE_SHARED0_RETURN_VALUE(hcclDB.database, HCCLSingleDeviceDB, false);
    std::string dbPath = Utils::GetDBPath({devicePath, SQLITE, hcclDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(hcclDB.dbRunner, DBRunner, false, dbPath);
    HcclStasticsFormat saveData;
    if (!Utils::Reserve(saveData, stasticsData->size())) {
        ERROR("Reserve for % data failed.", hcclDB.tableName);
        return false;
    }
    for (const auto& data : *stasticsData) {
        saveData.emplace_back(data.opType, data.count, data.totalTime, data.min, data.avg, data.max, data.ratio);
    }
    return SaveData(saveData, hcclDB, dbPath);
}

uint32_t DeviceHcclPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    INFO("Start to dump device hccl data.");
    const std::string devicePath = static_cast<const DeviceContext&>(context).GetDeviceFilePath();
    bool flag = true;
    if (!SaveHcclOpData(dataInventory, devicePath)) {
        flag = false;
        ERROR("Save hccl op single device data failed.");
    }
    if (!SaveHcclTaskData(dataInventory, devicePath)) {
        flag = false;
        ERROR("Save hccl task single device data failed.");
    }
    if (!SaveHcclStasticsData(dataInventory, devicePath)) {
        flag = false;
        ERROR("Save hccl stastics single device data failed.");
    }
    return (flag) ? ANALYSIS_OK : ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(DeviceHcclPersistence, true, HcclCalculator);
REGISTER_PROCESS_DEPENDENT_DATA(DeviceHcclPersistence, std::vector<HcclOp>, std::vector<DeviceHcclTask>,
                                std::vector<HcclStastics>);
REGISTER_PROCESS_SUPPORT_CHIP(DeviceHcclPersistence, CHIP_ID_ALL);
}
}