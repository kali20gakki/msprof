/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_proceser.h
 * Description        : task_processer，处理AscendTask表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/task_processer.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;
using namespace Analysis::Parser::Environment;
namespace {
struct TaskData {
    double start;
    double duration;
    double end;
    uint32_t connectionId;
    uint64_t correlationId;
    uint64_t taskType;
    uint32_t contextId;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t modelId;
    uint32_t batchId;
    TaskData() : start(0.0), duration(0.0), end(0.0), connectionId(UINT32_MAX), correlationId(UINT64_MAX),
        taskType(UINT64_MAX), contextId(UINT32_MAX), streamId(UINT32_MAX), taskId(UINT32_MAX), modelId(UINT32_MAX),
        batchId(UINT32_MAX)
    {}
};
}

TaskProcesser::TaskProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcesser(reportDBPath, profPaths)
{
    reportDB_.tableName = TABLE_NAME_TASK;
}

uint64_t TaskProcesser::GetTaskType(const std::string& hostType, const std::string& deviceType,
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
    return IdPool::GetInstance().GetUint64Id(taskType);
}

TaskProcesser::OriDataFormat TaskProcesser::GetData(DBInfo &ascendTaskDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT model_id, index_id, stream_id, task_id, context_id, batch_id, start_time, duration, "
                    "host_task_type, device_task_type, connection_id FROM " + ascendTaskDB.tableName +
                    " WHERE device_task_type <> 'UNKNOWN'"};
    if (!ascendTaskDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", ascendTaskDB.tableName);
    }
    return oriData;
}

TaskProcesser::ProcessedDataFormat TaskProcesser::FormatData(const OriDataFormat &oriData, uint16_t deviceId,
                                                             uint64_t globalPid, uint16_t platformVersion,
                                                             Utils::ProfTimeRecord &timeRecord)
{
    ProcessedDataFormat processedData;
    TaskData data;
    std::string oriHostTaskType;
    std::string oriDeviceTaskType;
    uint32_t oriConnectionId;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for ascend task data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.modelId, std::ignore, data.streamId, data.taskId, data.contextId, data.batchId,
                 data.start, data.duration, oriHostTaskType, oriDeviceTaskType, oriConnectionId) = row;
        data.end = data.start + std::max(data.duration, 0.0);
        data.connectionId = Utils::Splicing(globalPid, oriConnectionId);
        data.correlationId = IdPool::GetInstance().GetId(
            std::make_tuple(deviceId, data.modelId, data.streamId, data.taskId, data.contextId,
                            data.batchId));
        data.taskType = GetTaskType(oriHostTaskType, oriDeviceTaskType,
                                    platformVersion);
        processedData.emplace_back(
            std::to_string(Utils::GetLocalTime(data.start, timeRecord)),
            std::to_string(Utils::GetLocalTime(data.end, timeRecord)),
            deviceId, data.connectionId, data.correlationId, globalPid, data.taskType,
            data.contextId, data.streamId, data.taskId, data.modelId);
    }
    return processedData;
}

bool TaskProcesser::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    uint16_t deviceId;
    uint16_t platformVersion;
    uint32_t globalPid;
    Utils::ProfTimeRecord timeRecord;
    DBInfo ascendTaskDB;
    bool flag = true;
    ascendTaskDB.dbName = "ascend_task.db";
    ascendTaskDB.tableName = "AscendTask";
    MAKE_SHARED0_NO_OPERATION(ascendTaskDB.database, AscendTaskDB);
    globalPid = IdPool::GetInstance().GetUint32Id(fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        platformVersion = Context::GetInstance().GetPlatformVersion(deviceId, fileDir);
        if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir)) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            flag = false;
            continue;
        }
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ascendTaskDB.dbName});
        INFO("Start to process %, pid:%, deviceId:%.", dbPath, globalPid, deviceId);
        if (!Utils::FileReader::Check(dbPath, MAX_DB_BYTES)) {
            ERROR("DbPath does not exists: %.", dbPath);
            flag = false;
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(ascendTaskDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(ascendTaskDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", ascendTaskDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, deviceId, globalPid,
                                        platformVersion, timeRecord);
        if (!SaveData(processedData)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
        INFO("process %, pid:%, deviceId:% ends.", dbPath, globalPid, deviceId);
    }
    INFO("process % ends.", fileDir);
    return flag;
}

}
}
}