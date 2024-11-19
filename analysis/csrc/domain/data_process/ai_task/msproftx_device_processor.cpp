/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msprofTx_device_processor.cpp
 * Description        : 处理device侧stepTrace表的msprofTx数据
 * Author             : msprof team
 * Creation Date      : 2024/8/2
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/msproftx_device_processor.h"
#include <algorithm>
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"


namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
MsprofTxDeviceProcessor::MsprofTxDeviceProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool MsprofTxDeviceProcessor::ProcessOneDevice(const ProfTimeRecord &record, std::vector<MsprofTxDeviceData> &res,
                                               const std::string &devPath)
{
    DBInfo stepTraceDB("step_trace.db", "StepTrace");
    std::string dbPath = Utils::File::PathJoin({devPath, SQLITE, stepTraceDB.dbName});
    if (!stepTraceDB.ConstructDBRunner(dbPath)) {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, stepTraceDB);
    if (status != CHECK_SUCCESS) {
        if (status == CHECK_FAILED) {
            return false;
        }
        return true;
    }
    uint16_t deviceId = GetDeviceIdByDevicePath(devPath);
    if (deviceId == Parser::Environment::HOST_ID) {
        ERROR("the invalid deviceId cannot to be identified.");
        return false;
    }
    Utils::SyscntConversionParams params;
    if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_)) {
        ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
        return false;
    }
    auto oriData = LoadData(stepTraceDB, dbPath);
    if (oriData.empty()) {
        ERROR("StepTrace for msprofTx original data is empty. DBPath is %", dbPath);
        return false;
    }
    auto formatData = FormatData(oriData, record, deviceId, params);
    if (formatData.empty()) {
        ERROR("StepTrace for msprofTx data format failed, DBPath is %", dbPath);
        return false;
    }
    res.insert(res.end(), formatData.begin(), formatData.end());
    return true;
}

bool MsprofTxDeviceProcessor::Process(DataInventory &dataInventory)
{
    ProfTimeRecord record;
    if (!Context::GetInstance().GetProfTimeRecordInfo(record, profPath_)) {
        ERROR("GetProfTimeRecordInfo failed, profPath is %.", profPath_);
        return false;
    }
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<MsprofTxDeviceData> res;
    for (const auto& devicePath : deviceList) {
        flag = ProcessOneDevice(record, res, devicePath) && flag;
    }
    if (!SaveToDataInventory<MsprofTxDeviceData>(std::move(res), dataInventory, PROCESSOR_NAME_TASK)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_TASK);
        flag = false;
    }
    return flag;
}

OriMsprofTxDeviceData MsprofTxDeviceProcessor::LoadData(const DBInfo &stepTraceDB, const std::string &dbPath)
{
    OriMsprofTxDeviceData oriData;
    if (stepTraceDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{"SELECT model_id, index_id, stream_id, task_id, timestamp FROM " + stepTraceDB.tableName +
                    " WHERE tag_id = 11"};
    if (!stepTraceDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", stepTraceDB.tableName);
    }
    return oriData;
}

std::vector<MsprofTxDeviceData> MsprofTxDeviceProcessor::FormatData(
    OriMsprofTxDeviceData &oriData, const ProfTimeRecord &record, const uint16_t deviceId,
    const SyscntConversionParams &params)
{
    std::vector<MsprofTxDeviceData> processedData;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for AscendTask data failed.");
        return processedData;
    }
    MsprofTxDeviceData data;
    uint64_t start;
    data.deviceId = deviceId;
    std::sort(oriData.begin(), oriData.end(), [](TxDeviceData &lData, TxDeviceData rData) {
        if (std::get<1>(lData) != std::get<1>(rData)) { // 按照index_id、timestamp排序，即第1、4位
            return std::get<1>(lData) < std::get<1>(rData);  // 第1为为index_id
        } else {
            return std::get<4>(lData) < std::get<4>(rData); // 第4位为timestamp
        }
    });
    for (const auto& row : oriData) {
        std::tie(data.modelId, data.indexId, data.streamId, data.taskId, start) = row;
        data.connectionId = data.indexId + START_CONNECTION_ID_MSTX;
        HPFloat startTimestamp = Utils::GetTimeFromSyscnt(start, params);
        data.start = GetLocalTime(startTimestamp, record).Uint64();
        if (!processedData.empty() && data.indexId == processedData.back().indexId) {
            processedData.back().duration = static_cast<double>(data.start - processedData.back().start);
        } else {
            processedData.push_back(data);
        }
    }
    return processedData;
}
}
}
