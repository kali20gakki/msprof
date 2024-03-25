/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_processor.h
 * Description        : task_processor，处理AscendTask表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/task_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/association/credential/id_pool.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Association::Credential;
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
const std::string INDEX_NAME = "TaskIndex";
const std::vector<std::string> TASK_INDEX_COL_NAMES = {"startNs", "globalTaskId"};
struct TaskData {
    double start;
    double duration;
    double end;
    uint64_t connectionId;
    uint64_t globalTaskId;
    uint64_t taskType;
    uint32_t contextId;
    uint32_t streamId;
    uint32_t taskId;
    uint32_t modelId;
    uint32_t batchId;
    TaskData() : start(0.0), duration(0.0), end(0.0), connectionId(UINT32_MAX), globalTaskId(UINT64_MAX),
        taskType(UINT64_MAX), contextId(UINT32_MAX), streamId(UINT32_MAX), taskId(UINT32_MAX), modelId(UINT32_MAX),
        batchId(UINT32_MAX)
    {}
};
}

TaskProcessor::TaskProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool TaskProcessor::Run()
{
    INFO("TaskProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_TASK);
    return flag;
}

uint64_t TaskProcessor::GetTaskType(const std::string& hostType, const std::string& deviceType,
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

TaskProcessor::OriDataFormat TaskProcessor::GetData(DBInfo &ascendTaskDB)
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

TaskProcessor::ProcessedDataFormat TaskProcessor::FormatData(const OriDataFormat &oriData,
                                                             const ThreadData &threadData,
                                                             const uint16_t platformVersion, const uint32_t pid)
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
        HPFloat start{data.start};
        HPFloat end = start + HPFloat(data.duration);
        data.connectionId = Utils::Contact(threadData.profId, oriConnectionId);
        data.globalTaskId = IdPool::GetInstance().GetId(
            std::make_tuple(threadData.deviceId, data.streamId, data.taskId, data.contextId, data.batchId));
        data.taskType = GetTaskType(oriHostTaskType, oriDeviceTaskType,
                                    platformVersion);
        processedData.emplace_back(
            Utils::GetLocalTime(start, threadData.timeRecord).Uint64(),
            Utils::GetLocalTime(end, threadData.timeRecord).Uint64(),
            threadData.deviceId, data.connectionId, data.globalTaskId, pid, data.taskType,
            data.contextId, data.streamId, data.taskId, data.modelId);
    }
    return processedData;
}

bool TaskProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    ThreadData threadData;
    DBInfo ascendTaskDB("ascend_task.db", "AscendTask");
    bool flag = true;
    MAKE_SHARED0_NO_OPERATION(ascendTaskDB.database, AscendTaskDB);
    threadData.profId = IdPool::GetInstance().GetUint32Id(fileDir);
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ascendTaskDB.dbName});
        // 并不是所有场景都有ascend task数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        threadData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        uint16_t platformVersion = Context::GetInstance().GetPlatformVersion(threadData.deviceId, fileDir);
        if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir)) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            flag = false;
            continue;
        }
        INFO("Start to process %, pid:%, deviceId:%.", dbPath, pid, threadData.deviceId);
        MAKE_SHARED_RETURN_VALUE(ascendTaskDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(ascendTaskDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", ascendTaskDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, threadData, platformVersion, pid);
        if (!SaveData(processedData, TABLE_NAME_TASK)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
        if (!CreateTableIndex(TABLE_NAME_TASK, INDEX_NAME, TASK_INDEX_COL_NAMES)) {
            ERROR("Create table index failed.");
            flag = false;
        }
        INFO("process %, pid:%, deviceId:% ends.", dbPath, pid, threadData.deviceId);
    }
    INFO("process % ends.", fileDir);
    return flag;
}

}
}
}