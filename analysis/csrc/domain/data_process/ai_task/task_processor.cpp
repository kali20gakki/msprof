/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_task_processor.cpp
 * Description        : 处理ascendTask表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/2
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/task_processor.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
using IdPool = Analysis::Association::Credential::IdPool;
namespace {
// connection_id, data_size, memcpy_direction, max_size
using MemcpyInfoFormat = std::vector<std::tuple<uint64_t, uint64_t, uint16_t, uint64_t>>;
}

MemcpyRecord GenerateMemcpyRecordByConnectionId(MEMCPY_ASYNC_FORMAT &memcpyRecordMap, const uint64_t &connectionId)
{
    MemcpyRecord tmpMemcpyRecord {0, 0, 0, UINT16_MAX};
    auto it = memcpyRecordMap.find(connectionId);
    if (it == memcpyRecordMap.end()) {
        ERROR("MEMCPY_ASYNC task lost memcpyInfo, connectionId is %", connectionId);
        return tmpMemcpyRecord;
    }
    if (it->second.remainSize == 0) {
        ERROR("Redundant memcpyAsync task, connectionId is %", connectionId);
        return tmpMemcpyRecord;
    } else if (it->second.remainSize <= it->second.maxSize) {
        tmpMemcpyRecord.dataSize = it->second.remainSize;
        it->second.remainSize = 0;
    } else if (it->second.remainSize > it->second.maxSize) {
        tmpMemcpyRecord.dataSize = it->second.maxSize;
        it->second.remainSize = it->second.remainSize - tmpMemcpyRecord.dataSize;
    }
    if (it->second.operation <= VALID_MEMCPY_OPERATION) {
        tmpMemcpyRecord.operation = it->second.operation;
    }
    return tmpMemcpyRecord;
}

bool TaskProcessor::GenerateMemcpyRecordMap()
{
    DBInfo runtimeDB("runtime.db", "MemcpyInfo");
    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, runtimeDB.dbName});
    MAKE_SHARED_RETURN_VALUE(runtimeDB.dbRunner, DBRunner, false, dbPath);
    // 并不是所有场景都有memcpyInfo数据
    auto status = CheckPathAndTable(dbPath, runtimeDB, false);
    if (status == NOT_EXIST) {
        INFO("Check % data not exit in %.", runtimeDB.tableName, dbPath);
        return true;
    } else if (status == CHECK_FAILED) {
        ERROR("Check failed: %", dbPath);
        return false;
    }
    MemcpyInfoFormat memcpyInfoData;
    if (runtimeDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return false;
    }
    std::string sql = "SELECT connection_id, data_size, memcpy_direction, max_size FROM " + runtimeDB.tableName;
    if (!runtimeDB.dbRunner->QueryData(sql, memcpyInfoData)) {
        ERROR("Query memcpyInfo data failed, db path is %.", dbPath);
        return false;
    }
    if (memcpyInfoData.empty()) {
        ERROR("MemcpyInfo original data is empty.");
        return false;
    }
    uint64_t connectionId;
    uint64_t dataSize;
    uint16_t operation;
    uint64_t maxSize;
    for (const auto& data : memcpyInfoData) {
        std::tie(connectionId, dataSize, operation, maxSize) = data;
        memcpyRecordMap_[connectionId] = {dataSize, maxSize, dataSize, operation};
    }
    return true;
}

TaskProcessor::TaskProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool TaskProcessor::Process(DataInventory &dataInventory)
{
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    flag = GenerateMemcpyRecordMap();
    std::vector<AscendTaskData> allProcessedData;
    for (const auto& devicePath: deviceList) {
        flag = ProcessSingleDevice(devicePath, allProcessedData) && flag;
    }
    if (!SaveToDataInventory<AscendTaskData>(std::move(allProcessedData), dataInventory, PROCESSOR_NAME_TASK)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_TASK);
        flag = false;
    }
    if (!SaveToDataInventory<MemcpyInfoData>(std::move(processedMemcpyInfo_), dataInventory,
                                             PROCESSOR_NAME_MEMCPY_INFO)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_MEMCPY_INFO);
        flag = false;
    }
    return flag;
}

bool TaskProcessor::ProcessSingleDevice(const std::string &devicePath, std::vector<AscendTaskData> &allProcessedData)
{
    ProfTimeRecord record;
    DBInfo ascendTaskDB("ascend_task.db", "AscendTask");
    std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ascendTaskDB.dbName});
    if (!ascendTaskDB.ConstructDBRunner(dbPath)) {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, ascendTaskDB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
    if (deviceId == Parser::Environment::INVALID_DEVICE_ID) {
        ERROR("the invalid deviceId cannot to be identified.");
        return false;
    }
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    auto oriData = LoadData(ascendTaskDB, dbPath);
    if (oriData.empty()) {
        ERROR("AscendTask original data is empty. DBPath is %", dbPath);
        return false;
    }
    auto formatData = FormatData(oriData, record, deviceId);
    if (formatData.empty()) {
        ERROR("AscendTask format data failed. DBPath is %", dbPath);
        return false;
    }
    allProcessedData.insert(allProcessedData.end(), formatData.begin(), formatData.end());
    return true;
}

OriAscendTaskData TaskProcessor::LoadData(const DBInfo &ascendTaskDB, const std::string &dbPath)
{
    OriAscendTaskData oriData;
    if (ascendTaskDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{"SELECT a.start_time, a.duration, a.model_id, a.index_id, a.stream_id, a.task_id, a.context_id,"
                    "a.batch_id, a.connection_id, a.host_task_type, a.device_task_type FROM " + ascendTaskDB.tableName +
                    " a WHERE a.device_task_type != 'UNKNOWN'"};
    if (!ascendTaskDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", ascendTaskDB.tableName);
    }
    return oriData;
}

std::vector<AscendTaskData> TaskProcessor::FormatData(
    const OriAscendTaskData &oriData, const ProfTimeRecord &timeRecord, const uint16_t deviceId)
{
    std::vector<AscendTaskData> processedData;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for AscendTask data failed.");
        return processedData;
    }
    bool memcpyFalg = true;
    if (memcpyRecordMap_.empty()) {
        memcpyFalg = false;
    }
    AscendTaskData data;
    MemcpyInfoData memcpyInfoData;
    data.deviceId = deviceId;
    double tmpStart;
    uint16_t platformVersion = Context::GetInstance().GetPlatformVersion(deviceId, profPath_);
    for (const auto &row: oriData) {
        std::tie(tmpStart, data.duration, data.modelId, data.indexId, data.streamId, data.taskId, data.contextId,
                 data.batchId, data.connectionId, data.hostType, data.deviceType) = row;
        HPFloat start{tmpStart};
        HPFloat end = start + HPFloat(data.duration);
        data.start = GetLocalTime(start, timeRecord).Uint64();
        data.end = GetLocalTime(end, timeRecord).Uint64();
        data.taskType = GetTaskType(data.hostType, data.deviceType, platformVersion);
        processedData.push_back(data);
        if (memcpyFalg && data.hostType == MEMCPY_ASYNC) {
            MemcpyRecord tmpMemcpyRecord = GenerateMemcpyRecordByConnectionId(memcpyRecordMap_, data.connectionId);
            memcpyInfoData.globalTaskId = IdPool::GetInstance().GetId(std::make_tuple(data.deviceId, data.streamId,
                data.taskId, data.contextId, data.batchId));
            memcpyInfoData.dataSize = tmpMemcpyRecord.dataSize;
            memcpyInfoData.memcpyOperation = tmpMemcpyRecord.operation;
            processedMemcpyInfo_.emplace_back(memcpyInfoData);
        }
    }
    return processedData;
}

std::string TaskProcessor::GetTaskType(const std::string& hostType, const std::string& deviceType,
                                       uint16_t platformVersion)
{
    std::string taskType;
    std::map<std::string, std::string> sqeType;
    if (Context::IsStarsChip(platformVersion)) {
        sqeType = STARS_SQE_TYPE_TABLE;
    } else {
        sqeType = HW_SQE_TYPE_TABLE;
    }
    if (hostType != "FFTS_PLUS" && hostType != "UNKNOWN") {
        taskType = hostType;
    } else if (hostType == "FFTS_PLUS") {
        taskType = deviceType;
    } else {
        if (!Utils::IsNumber(deviceType)) {
            taskType = deviceType;
        } else if (platformVersion != static_cast<int>(Chip::CHIP_V2_1_0) &&
        sqeType.find(deviceType) != sqeType.end()) {
            taskType = sqeType[deviceType];
        } else {
            taskType = "Other";
        }
    }
    return taskType;
}
}
}
