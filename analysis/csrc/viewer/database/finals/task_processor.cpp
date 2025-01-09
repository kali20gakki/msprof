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
#include <algorithm>
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

struct MsprofTxTaskData {
    uint32_t modelId = UINT32_MAX;
    uint32_t indexId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    double timestamp = 0.0;

    bool operator<(const MsprofTxTaskData& other) const
    {
        if (indexId < other.indexId) {
            return true;
        } else if (indexId == other.indexId) {
            return timestamp < other.timestamp;
        } else {
            return false;
        }
    }
};

struct MsprofTxTableData {
    uint32_t modelId = UINT32_MAX;
    uint32_t indexId = UINT32_MAX;
    uint32_t streamId = UINT32_MAX;
    uint32_t taskId = UINT32_MAX;
    double startTime = 0.0;
    double endTime = 0.0;

    MsprofTxTableData(uint32_t modelId, uint32_t indexId, uint32_t streamId, uint32_t taskId,
                      double startTime, double endTime)
    {
        this->modelId = modelId;
        this->indexId = indexId;
        this->streamId = streamId;
        this->taskId = taskId;
        this->startTime = startTime;
        this->endTime = endTime;
    }
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
    INFO("Start to task process %.", fileDir);
    ThreadData threadData;
    threadData.profId = IdPool::GetInstance().GetUint32Id(fileDir);
    uint32_t pid = Context::GetInstance().GetPidFromInfoJson(Parser::Environment::HOST_ID, fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);

    bool processFlag = ProcessTaskData(fileDir, deviceList, threadData, pid);
    processFlag = ProcessMsprofTxData(fileDir, deviceList, threadData, pid) && processFlag;
    return processFlag;
}

bool TaskProcessor::ProcessTaskData(const std::string &fileDir, const std::vector<std::string> &deviceList,
                                    ThreadData threadData, uint32_t pid)
{
    INFO("Start to task data process %.", fileDir);
    bool flag = true;
    for (const auto& devicePath: deviceList) {
        DBInfo ascendTaskDB("ascend_task.db", "AscendTask");
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, ascendTaskDB.dbName});
        MAKE_SHARED0_NO_OPERATION(ascendTaskDB.database, AscendTaskDB);
        MAKE_SHARED_RETURN_VALUE(ascendTaskDB.dbRunner, DBRunner, false, dbPath);
        // 并不是所有场景都有ascend task数据
        auto status = CheckPathAndTable(dbPath, ascendTaskDB);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        threadData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir, threadData.deviceId)) {
            ERROR("Failed to obtain the time in start_info and end_info. "
                  "Path is %, device id is %.", fileDir, threadData.deviceId);
            flag = false;
            continue;
        }
        uint16_t platformVersion = Context::GetInstance().GetPlatformVersion(threadData.deviceId, fileDir);
        INFO("Start to process %, pid:%, deviceId:%.", dbPath, pid, threadData.deviceId);
        auto oriData = GetData(ascendTaskDB);
        if (oriData.empty()) {
            ERROR("Get % data failed in %.", ascendTaskDB.tableName, dbPath);
            flag = false;
            continue;
        }
        auto processedData = FormatData(oriData, threadData, platformVersion, pid);
        if (!SaveData(processedData, TABLE_NAME_TASK)) {
            ERROR("Save data failed, %.", dbPath);
            flag = false;
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

bool TaskProcessor::ProcessMsprofTxData(const std::string &fileDir, const std::vector<std::string> &deviceList,
                                        ThreadData threadData, uint32_t pid)
{
    INFO("start msprof tx process");
    bool flag = true;
    for (auto& devicePath: deviceList) {
        DBInfo stepTraceDB("step_trace.db", "StepTrace");
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, stepTraceDB.dbName});
        MAKE_SHARED0_NO_OPERATION(stepTraceDB.database, StepTraceDB);
        MAKE_SHARED_RETURN_VALUE(stepTraceDB.dbRunner, DBRunner, false, dbPath);
        // 并不是所有场景都有msproftx 打点 task数据
        auto status = CheckPathAndTable(dbPath, stepTraceDB);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        threadData.deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir, threadData.deviceId)) {
            ERROR("Failed to obtain the time in start_info and end_info. "
                  "Path is %, device id is %.", fileDir, threadData.deviceId);
            flag = false;
            continue;
        }
        Utils::SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, threadData.deviceId, fileDir)) {
            ERROR("GetSyscntConversionParams failed, profPath is %.", fileDir);
            flag = false;
            continue;
        }
        INFO("Start to process %, pid:%, deviceId:%.", dbPath, pid, threadData.deviceId);
        auto oriData = GetMsprofTxTaskData(stepTraceDB);
        if (oriData.empty()) {
            WARN("Get % data failed in %.", stepTraceDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatMsprofTxTaskData(oriData, threadData, params, pid);
        if (!SaveData(processedData, TABLE_NAME_TASK)) {
            ERROR("Save data failed, %.", dbPath);
            flag = false;
            continue;
        }
        if (!CreateTableIndex(TABLE_NAME_TASK, INDEX_NAME, TASK_INDEX_COL_NAMES)) {
            ERROR("Create table index failed.");
            flag = false;
        }
        INFO("process %, pid:%, deviceId:% ends.", dbPath, pid, threadData.deviceId);
    }
    return flag;
}

TaskProcessor::OriMsprofTxDataFormat TaskProcessor::GetMsprofTxTaskData(DBInfo &stepTraceDB)
{
    OriMsprofTxDataFormat oriData;
    std::string sql{"SELECT model_id, index_id, stream_id, task_id, timestamp FROM " + stepTraceDB.tableName +
                    " WHERE tag_id = 11"};
    if (!stepTraceDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", stepTraceDB.tableName);
    }
    return oriData;
}

TaskProcessor::ProcessedDataFormat TaskProcessor::FormatMsprofTxTaskData(const OriMsprofTxDataFormat &oriData,
                                                                         const ThreadData &threadData,
                                                                         Utils::SyscntConversionParams &params,
                                                                         const uint32_t pid)
{
    uint64_t taskType = IdPool::GetInstance().GetUint64Id("MsTx");
    ProcessedDataFormat processedData;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for ascend task data failed.");
        return processedData;
    }
    std::vector<MsprofTxTaskData> dataList;
    MsprofTxTaskData data;
    for (auto &row: oriData) {
        std::tie(data.modelId, data.indexId, data.streamId, data.taskId, data.timestamp) = row;
        dataList.emplace_back(data);
    }
    std::sort(dataList.begin(), dataList.end());
    std::vector<MsprofTxTableData> tableDataList;
    tableDataList.emplace_back(MsprofTxTableData(dataList[0].modelId, dataList[0].indexId,
                                                 dataList[0].streamId, dataList[0].taskId,
                                                 dataList[0].timestamp, dataList[0].timestamp));
    for (size_t i = 1; i < dataList.size(); i++) {
        if (dataList[i].indexId == dataList[i - 1].indexId) {
            tableDataList.back().endTime = dataList[i].timestamp;
        } else {
            tableDataList.emplace_back(MsprofTxTableData(dataList[i].modelId, dataList[i].indexId,
                                                         dataList[i].streamId, dataList[i].taskId,
                                                         dataList[i].timestamp, dataList[i].timestamp));
        }
    }
    for (auto &data : tableDataList) {
        uint64_t connectionId = Utils::Contact(threadData.profId, data.indexId + START_CONNECTION_ID_MSTX);
        uint64_t globalTaskId = IdPool::GetInstance().GetId(std::make_tuple(threadData.deviceId, data.streamId,
                                                                            data.taskId, UINT32_MAX,
                                                                            data.indexId + START_CONNECTION_ID_MSTX));
        HPFloat startTimestamp = Utils::GetTimeFromSyscnt(static_cast<uint64_t>(data.startTime), params);
        HPFloat endTimestamp = Utils::GetTimeFromSyscnt(static_cast<uint64_t>(data.endTime), params);
        processedData.emplace_back(
            Utils::GetLocalTime(startTimestamp, threadData.timeRecord).Uint64(),
            Utils::GetLocalTime(endTimestamp, threadData.timeRecord).Uint64(),
            threadData.deviceId, connectionId, globalTaskId, pid, taskType,
            UINT32_MAX, data.streamId, data.taskId, data.modelId);
    }
    return processedData;
}

}
}
}