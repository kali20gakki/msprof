/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_processor.h
 * Description        : 处理kfcinfo表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/26
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/kfc_task_processor.h"
#include <string>
#include <functional>
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

// 创建 KfcTurnData 的函数类型
using CreateKfcTurnDataFunc = std::function<KfcTurnData(const KfcCommTurn&, const SyscntConversionParams&)>;

KfcTaskProcessor::KfcTaskProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool KfcTaskProcessor::Process(DataInventory& dataInventory)
{
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<KfcTurnData> result;
    bool flag = true;
    for (const auto& devicePath: deviceList) {
        DBInfo kfcCommTurnDB("kfc_info.db", "KfcCommTurn");
        DBInfo kfcComputeTurnDB("kfc_info.db", "KfcComputeTurn");
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, kfcComputeTurnDB.dbName});
        if (!kfcCommTurnDB.ConstructDBRunner(dbPath) || !kfcComputeTurnDB.ConstructDBRunner(dbPath)) {
            flag = false;
            continue;
        }

        auto commStatus = CheckPathAndTable(dbPath, kfcCommTurnDB, false);
        auto computeStatus = CheckPathAndTable(dbPath, kfcComputeTurnDB, false);
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        if (computeStatus == CHECK_SUCCESS) {
            auto computeData = LoadComputeData(kfcComputeTurnDB, dbPath);
            std::vector<KfcTurnData> kfcComputeTurnData = FormatComputeData(computeData, deviceId);
            result.insert(result.end(), kfcComputeTurnData.begin(), kfcComputeTurnData.end());
        } else {
            flag = (computeStatus == NOT_EXIST) && flag;
        }
        if (commStatus == CHECK_SUCCESS) {
            auto commData = LoadCommData(kfcCommTurnDB, dbPath);
            std::vector<KfcTurnData> kfcCommTurnData = FormatCommData(commData, deviceId);
            result.insert(result.end(), kfcCommTurnData.begin(), kfcCommTurnData.end());
        } else {
            flag = (commStatus == NOT_EXIST) && flag;
        }
    }

    if (!SaveToDataInventory<KfcTurnData>(std::move(result), dataInventory, PROCESSOR_NAME_KFC_TASK)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_KFC_TASK);
        flag = false;
    }
    return flag;
}

std::vector<KfcCommTurn> KfcTaskProcessor::LoadCommData(const DBInfo &kfcDB, const std::string &dbPath)
{
    std::vector<KfcCommTurn> kfcCommTurnData;
    OriKfcCommData oriData;
    if (kfcDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return kfcCommTurnData;
    }
    std::string sql = "SELECT device_id, stream_id, task_id, comm_turn, current_turn, wait_notify_start_time, "
                      "kfc_alg_exe_start_time, send_task_start_time,wait_active_start_time,active_start_time, "
                      "wait_exe_end_start_time, rtsq_exe_end_time "
                      "FROM " + kfcDB.tableName;
    if (!kfcDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query api data failed, db path is %.", dbPath);
        return kfcCommTurnData;
    }
    for (auto row : oriData) {
        KfcCommTurn data{};
        std::tie(data.deviceId, data.streamId, data.taskId, data.commTurn, data.currentTurn, data.waitNotifyStartTime,
                 data.kfcAlgExeStartTime, data.sendTaskStartTime, data.waitActiveStartTime, data.activeStartTime,
                 data.waitExeEndStartTime, data.rtsqExeEndTime) = row;
        kfcCommTurnData.push_back(data);
    }

    return kfcCommTurnData;
}

std::vector<KfcComputeTurn> KfcTaskProcessor::LoadComputeData(const DBInfo &kfcDB, const std::string &dbPath)
{
    OriKfcComputeData oriData;
    std::vector<KfcComputeTurn> kfcComputeDatas;
    if (kfcDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return kfcComputeDatas;
    }
    std::string sql = "SELECT device_id, stream_id, task_id, compute_turn, current_turn, wait_compute_start_time, "
                      "compute_start_time, compute_exe_end_time "
                      "FROM " + kfcDB.tableName;
    if (!kfcDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query api data failed, db path is %.", dbPath);
        return kfcComputeDatas;
    }
    for (auto row : oriData) {
        KfcComputeTurn data{};
        std::tie(data.deviceId, data.streamId, data.taskId, data.computeTurn, data.currentTurn,
                 data.waitComputeStartTime, data.computeStartTime, data.computeExeEndTime) = row;
        kfcComputeDatas.push_back(data);
    }
    return kfcComputeDatas;
}

std::vector<KfcTurnData> KfcTaskProcessor::FormatCommData(const std::vector<KfcCommTurn> &oriCommData,
                                                          uint16_t deviceId)
{
    SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_)) {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return {};
    }
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, deviceId);
        return {};
    }
    std::vector<KfcTurnData> ans;
    HPFloat startTimestamp;
    for (const auto& commData : oriCommData) {
        startTimestamp = GetTimeFromSyscnt(commData.waitNotifyStartTime, params);
        ans.emplace_back("WaitNotify " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.kfcAlgExeStartTime - commData.waitNotifyStartTime, params)
                         .Double());
        startTimestamp = GetTimeFromSyscnt(commData.kfcAlgExeStartTime, params);
        ans.emplace_back("KfcAlgExe " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.sendTaskStartTime - commData.kfcAlgExeStartTime, params)
                         .Double());
        startTimestamp = GetTimeFromSyscnt(commData.sendTaskStartTime, params);
        ans.emplace_back("SendTask " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.waitActiveStartTime - commData.sendTaskStartTime, params)
                         .Double());
        startTimestamp = GetTimeFromSyscnt(commData.waitActiveStartTime, params);
        ans.emplace_back("WaitActive " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.activeStartTime - commData.sendTaskStartTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(commData.activeStartTime, params);
        ans.emplace_back("Active " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.waitExeEndStartTime - commData.activeStartTime, params).
                         Double());
        startTimestamp = GetTimeFromSyscnt(commData.waitExeEndStartTime, params);
        ans.emplace_back("WaitExeEnd " + std::to_string(commData.currentTurn), commData.deviceId, commData.streamId,
                         commData.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(commData.rtsqExeEndTime - commData.waitExeEndStartTime, params)
                         .Double());
    }
    return ans;
}

std::vector<KfcTurnData> KfcTaskProcessor::FormatComputeData(const std::vector<KfcComputeTurn> &oriComputeData,
                                                             uint16_t deviceId)
{
    SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_)) {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return {};
    }
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_, deviceId)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %, device id is %.", profPath_, deviceId);
        return {};
    }
    std::vector<KfcTurnData> ans;
    HPFloat startTimestamp;
    for (auto& data : oriComputeData) {
        startTimestamp = GetTimeFromSyscnt(data.waitComputeStartTime, params);
        ans.emplace_back("WaitCompute " + std::to_string(data.currentTurn), deviceId, data.streamId,
                         data.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(data.computeStartTime - data.waitComputeStartTime, params).Double());
        startTimestamp = GetTimeFromSyscnt(data.computeStartTime, params);
        ans.emplace_back("Compute " + std::to_string(data.currentTurn), deviceId, data.streamId,
                         data.taskId, GetLocalTime(startTimestamp, record).Uint64(),
                         GetDurTimeFromSyscnt(data.computeExeEndTime - data.computeStartTime, params).Double());
    }
    return ans;
}
}
}