/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : db_assembler.cpp
 * Description        : DB数据组合类
 * Author             : msprof team
 * Creation Date      : 2024/8/27
 * *****************************************************************************
 */


#include "analysis/csrc/application/database/db_assembler.h"
#include <functional>

#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/acc_pmu_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/aicore_freq_data.h"
#include "analysis/csrc/domain/entities/viewer_data/system//include/ddr_data.h"


namespace Analysis {
namespace Application {
using namespace Analysis::Domain;
using namespace Analysis::Utils;
using namespace Analysis::Parser::Environment;
using IdPool = Analysis::Association::Credential::IdPool;

// 创建 SaveData 的函数类型
using SaveDataFunc = std::function<bool(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)>;

bool SaveApiData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, profPath);
    auto apiData = dataInventory.GetPtr<std::vector<ApiData>>();
    if (apiData == nullptr) {
        WARN("Api data not exist.");
        return true;
    }
    std::vector<std::tuple<uint64_t, uint64_t, uint16_t, uint64_t, uint64_t, uint64_t>> res;
    if (!Reserve(res, apiData->size())) {
        ERROR("Reserved for api data failed.");
        return false;
    }
    for (const auto& item : *apiData) {
        uint64_t name = IdPool::GetInstance().GetUint64Id(item.apiName);
        uint64_t globalTid = Utils::Contact(pid, item.threadId);
        res.emplace_back(item.start, item.end, item.level, globalTid, item.connectionId, name);
    }
    return SaveData(res, TABLE_NAME_CANN_API, msprofDB);
}

bool SaveCommOpData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // 大算子数据
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
        int32_t, int32_t, int32_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
    auto opData = dataInventory.GetPtr<std::vector<CommunicationOpData>>();
    if (opData == nullptr) {
        WARN("Communication op data not exist.");
        return true;
    }
    CommunicationOpDataFormat processedOpData;
    if (!Reserve(processedOpData, opData->size())) {
        ERROR("Reserved for communication op data failed.");
        return false;
    }
    for (const auto& item : *opData) {
        uint64_t groupName = IdPool::GetInstance().GetUint64Id(item.groupName);
        uint64_t opName = IdPool::GetInstance().GetUint64Id(item.opName);
        int32_t opId = IdPool::GetInstance().GetUint32Id(item.opKey);
        uint64_t algType = IdPool::GetInstance().GetUint64Id(item.algType);
        uint64_t opType = IdPool::GetInstance().GetUint64Id(item.opType);
        processedOpData.emplace_back(opName, item.start, item.end, item.connectionId, groupName, opId, item.relay,
                                     item.retry, item.dataType, algType, item.count, opType);
    }
    return SaveData(processedOpData, TABLE_NAME_COMMUNICATION_OP, msprofDB);
}

bool SaveCommTaskData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // 小算子数据
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint32_t, uint32_t, uint64_t,
        uint64_t, uint64_t, uint64_t, uint32_t>>;
    auto taskData = dataInventory.GetPtr<std::vector<CommunicationTaskData>>();
    if (taskData == nullptr) {
        WARN("Communication task data not exist.");
        return true;
    }
    CommunicationTaskDataFormat processedTaskData;
    if (!Reserve(processedTaskData, taskData->size())) {
        ERROR("Reserved for communication task failed.");
        return false;
    }
    for (const auto& item : *taskData) {
        uint64_t groupName = IdPool::GetInstance().GetUint64Id(item.groupName);
        uint64_t opName = IdPool::GetInstance().GetUint64Id(item.opName);
        uint64_t taskType = IdPool::GetInstance().GetUint64Id(item.taskType);
        uint64_t globalTaskId = IdPool::GetInstance().GetId(
            std::make_tuple(item.deviceId, item.streamId, item.taskId, item.contextId, item.batchId));
        uint32_t opId = IdPool::GetInstance().GetUint32Id(item.opKey);
        processedTaskData.emplace_back(opName, globalTaskId, taskType, item.planeId,
                                       groupName, item.notifyId, item.rdmaType, item.srcRank,
                                       item.dstRank, item.transportType, item.size, item.dataType,
                                       item.linkType, opId);
    }
    return SaveData(processedTaskData, TABLE_NAME_COMMUNICATION_TASK_INFO, msprofDB);
}

bool SaveCommunicationData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    bool saveFlag = SaveCommOpData(dataInventory, msprofDB, profPath);
    saveFlag = SaveCommTaskData(dataInventory, msprofDB, profPath) && saveFlag;
    return saveFlag;
}

bool SaveAccPmuData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // accId, readBwLevel, writeBwLevel, readOstLevel, writeOstLevel, timestampNs, deviceId
    using AccPmuDataFormat = std::vector<std::tuple<uint16_t, uint32_t, uint32_t,
        uint32_t, uint32_t, uint64_t, uint16_t>>;
    auto accPmuData = dataInventory.GetPtr<std::vector<AccPmuData>>();
    if (accPmuData == nullptr) {
        WARN("Acc pmu data not exist.");
        return true;
    }
    AccPmuDataFormat res;
    if (!Reserve(res, accPmuData->size())) {
        ERROR("Reserved for acc pmu data failed.");
        return false;
    }
    for (const auto& item : *accPmuData) {
        res.emplace_back(item.accId, item.readBwLevel, item.writeBwLevel, item.readOstLevel,
                         item.writeOstLevel, item.timestampNs, item.deviceId);
    }
    return SaveData(res, TABLE_NAME_ACC_PMU, msprofDB);
}

bool SaveAicoreFreqData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, timestampNs, freq
    using AicoreFreqDataFormat = std::vector<std::tuple<uint16_t, uint64_t, double>>;
    auto aicoreFreqData = dataInventory.GetPtr<std::vector<AicoreFreqData>>();
    if (aicoreFreqData == nullptr) {
        WARN("AIcore freq data not exist.");
        return true;
    }
    AicoreFreqDataFormat res;
    if (!Reserve(res, aicoreFreqData->size())) {
        ERROR("Reserved for AIcore freq data failed.");
        return false;
    }
    for (const auto& item : *aicoreFreqData) {
        res.emplace_back(item.deviceId, item.timestamp, item.freq);
    }
    return SaveData(res, TABLE_NAME_AICORE_FREQ, msprofDB);
}

bool SaveDDRData(DataInventory& dataInventory, DBInfo& msprofDB, const std::string& profPath)
{
    // deviceId, timestamp, read, write
    using DDRDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t>>;
    auto ddrData = dataInventory.GetPtr<std::vector<DDRData>>();
    if (ddrData == nullptr) {
        WARN("DDR data not exist.");
        return true;
    }
    DDRDataFormat res;
    if (!Reserve(res, ddrData->size())) {
        ERROR("Reserved for DDR data failed.");
        return false;
    }
    for (const auto& item : *ddrData) {
        res.emplace_back(item.deviceId, item.timestamp, static_cast<uint64_t>(item.fluxRead),
                         static_cast<uint64_t>(item.fluxWrite));
    }
    return SaveData(res, TABLE_NAME_DDR, msprofDB);
}

std::unordered_map<std::string, SaveDataFunc> dataSaver = {
    {Viewer::Database::PROCESSOR_NAME_API,           SaveApiData},
    {Viewer::Database::PROCESSOR_NAME_COMMUNICATION, SaveCommunicationData},
    {Viewer::Database::PROCESSOR_NAME_ACC_PMU,       SaveAccPmuData},
    {Viewer::Database::PROCESSOR_NAME_AICORE_FREQ,   SaveAicoreFreqData},
    {Viewer::Database::PROCESSOR_NAME_DDR,           SaveDDRData},
};

DBAssembler::DBAssembler(
    const std::string& msprofDBPath,
    const std::string& profPath)
    : msprofDBPath_(msprofDBPath), profPath_(profPath)
{
    MAKE_SHARED0_NO_OPERATION(msprofDB_.database, MsprofDB);
    msprofDB_.ConstructDBRunner(msprofDBPath_);
}

DBAssembler::DBAssembler(
    const std::string& msprofDBPath)
    : msprofDBPath_(msprofDBPath)
{
    MAKE_SHARED0_NO_OPERATION(msprofDB_.database, MsprofDB);
    msprofDB_.ConstructDBRunner(msprofDBPath_);
}

bool DBAssembler::Run(DataInventory& dataInventory, const std::string& processorName)
{
    INFO("Begin to Assemble % data", processorName);
    bool flag = true;
    for (const auto& saveFunc : dataSaver) {
        flag = saveFunc.second(dataInventory, msprofDB_, profPath_) && flag;
    }
    return flag;
}

}
}