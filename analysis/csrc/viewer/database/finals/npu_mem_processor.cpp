/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_mem_processor.cpp
 * Description        : npu_mem_processor，处理NpuMem表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/7
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/npu_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct NpuMemData {
    uint64_t ddr = 0;
    uint64_t hbm = 0;
    double timestamp = 0.0;
    std::string event;
};
}

NpuMemProcessor::NpuMemProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths) {}

bool NpuMemProcessor::Run()
{
    INFO("NpuMemProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_NPU_MEM);
    return flag;
}

NpuMemProcessor::OriDataFormat NpuMemProcessor::GetData(DBInfo &npuMemDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT event, ddr, hbm, timestamp FROM " + npuMemDB.tableName};
    if (!npuMemDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", npuMemDB.tableName);
    }
    return oriData;
}

NpuMemProcessor::ProcessedDataFormat NpuMemProcessor::FormatData(const OriDataFormat &oriData, uint16_t deviceId,
                                                                 const Utils::ProfTimeRecord &timeRecord,
                                                                 Utils::SyscntConversionParams &params)
{
    ProcessedDataFormat processedData;
    NpuMemData data;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for NpuMem data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.event, data.ddr, data.hbm, data.timestamp) = row;
        HPFloat timestamp{GetTimeBySamplingTimestamp(data.timestamp, params)};
        uint16_t type = UINT16_MAX;
        if (Utils::StrToU16(type, data.event) == ANALYSIS_ERROR) {
            WARN("Converting string(event) to integer failed.");
        }
        processedData.emplace_back(type, data.ddr, data.hbm, GetLocalTime(timestamp, timeRecord).Uint64(), deviceId);
    }
    return processedData;
}

bool NpuMemProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    Utils::ProfTimeRecord timeRecord;
    Utils::SyscntConversionParams params;
    DBInfo npuMemDB("npu_mem.db", "NpuMem");
    bool flag = true;
    MAKE_SHARED0_NO_OPERATION(npuMemDB.database, NpuMemDB);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool timeFlag = Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, fileDir);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, npuMemDB.dbName});
        // 并不是所有场景都有NpuMem数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        if (!timeFlag) {
            ERROR("GetSyscntConversionParams failed, profPath is %.", fileDir);
            return false;
        }
        uint16_t deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        INFO("Start to process %, deviceId:%.", dbPath, deviceId);
        if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir)) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            flag = false;
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(npuMemDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(npuMemDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", npuMemDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, deviceId, timeRecord, params);
        if (!SaveData(processedData, TABLE_NAME_NPU_MEM)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
        INFO("process %, deviceId:% ends.", dbPath, deviceId);
    }
    INFO("process % ends.", fileDir);
    return flag;
}

}
}
}