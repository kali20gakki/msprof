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
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
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

        auto commStatus = CheckPathAndTable(dbPath, kfcCommTurnDB);
        auto computeStatus = CheckPathAndTable(dbPath, kfcComputeTurnDB);
        if (commStatus != CHECK_SUCCESS && computeStatus != CHECK_SUCCESS) {
            if (commStatus == CHECK_FAILED || computeStatus == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }

        // db取出所有的数据
        auto commData = LoadCommData(kfcCommTurnDB, dbPath);
        auto computeData = LoadComputeData(kfcComputeTurnDB, dbPath);
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        // 完成format
        std::vector<KfcTurnData> kfcCommTurnData = FormatCommData(commData, deviceId);
        result.insert(result.end(), kfcCommTurnData.begin(), kfcCommTurnData.end());
        std::vector<KfcTurnData> kfcComputeTurnData = FormatComputeData(computeData, deviceId);
        result.insert(result.end(), kfcComputeTurnData.begin(), kfcComputeTurnData.end());
    }

    if (!SaveToDataInventory<KfcTurnData>(std::move(result), dataInventory, PROCESSOR_NAME_TASK)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_TASK);
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
    std::vector<KfcTurnData> ans;

    for (const auto& commData : oriCommData) {
        ans.emplace_back("WaitNotify " + commData.currentTurn, commData.deviceId, commData.streamId,
                         commData.taskId, GetTimeFromSyscnt(commData.waitNotifyStartTime, params).Uint64(),
                         GetDurTimeFromSyscnt(commData.kfcAlgExeStartTime - commData.waitNotifyStartTime, params)
                         .Double());
        ans.emplace_back("KfcAlgExe " + commData.currentTurn, commData.deviceId, commData.streamId,
                         commData.taskId, GetTimeFromSyscnt(commData.kfcAlgExeStartTime, params).Uint64(),
                         GetDurTimeFromSyscnt(commData.sendTaskStartTime - commData.kfcAlgExeStartTime, params)
                         .Double());
        ans.emplace_back("SendTask " + commData.currentTurn, commData.deviceId, commData.streamId,
                         commData.taskId, GetTimeFromSyscnt(commData.sendTaskStartTime, params).Uint64(),
                         GetDurTimeFromSyscnt(commData.waitActiveStartTime - commData.sendTaskStartTime, params)
                         .Double());
        ans.emplace_back("WaitActive " + commData.currentTurn, commData.deviceId, commData.streamId,
                         commData.taskId, GetTimeFromSyscnt(commData.waitActiveStartTime, params).Uint64(),
                         GetDurTimeFromSyscnt(commData.activeStartTime - commData.sendTaskStartTime, params).Double());
        ans.emplace_back("Active " + commData.currentTurn, commData.deviceId, commData.streamId, commData.taskId,
                         GetTimeFromSyscnt(commData.activeStartTime, params).Uint64(),
                         GetDurTimeFromSyscnt(commData.waitExeEndStartTime - commData.activeStartTime, params)
                         .Double());
        ans.emplace_back("WaitExeEnd " + commData.currentTurn, commData.deviceId, commData.streamId,
                         commData.taskId, GetTimeFromSyscnt(commData.waitExeEndStartTime, params).Uint64(),
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
    std::vector<KfcTurnData> ans;
    for (auto& data : oriComputeData) {
        ans.emplace_back("WaitCompute " + data.currentTurn, deviceId, data.streamId,
                         data.taskId, GetTimeFromSyscnt(data.waitComputeStartTime, params).Uint64(),
                         GetDurTimeFromSyscnt(data.computeStartTime - data.waitComputeStartTime, params).Double());
        ans.emplace_back("Compute " + data.currentTurn, deviceId, data.streamId,
                         data.taskId, GetTimeFromSyscnt(data.computeStartTime, params).Uint64(),
                         GetDurTimeFromSyscnt(data.computeExeEndTime - data.computeStartTime, params).Double());
    }
    return ans;
}
}
}