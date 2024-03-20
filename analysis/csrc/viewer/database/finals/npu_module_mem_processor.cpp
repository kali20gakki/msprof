/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : npu_module_mem_processor.cpp
 * Description        : npu_module_mem_processor，处理NpuModuleMem表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/2/17
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/npu_module_mem_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct NpuModuleMemData {
    uint32_t moduleId = 0;
    uint64_t totalSize = 0;
    double syscnt = 0.0;
    std::string deviceType;
};
}

NpuModuleMemProcessor::NpuModuleMemProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(reportDBPath, profPaths)
{}

bool NpuModuleMemProcessor::Run()
{
    INFO("NpuModuleMemProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_NPU_MODULE_MEM);
    return flag;
}

NpuModuleMemProcessor::OriDataFormat NpuModuleMemProcessor::GetData(DBInfo &npuModuleMemDB)
{
    OriDataFormat oriData;
    std::string sql{"SELECT module_id, syscnt, total_size, device_type FROM " + npuModuleMemDB.tableName};
    if (!npuModuleMemDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", npuModuleMemDB.tableName);
    }
    return oriData;
}

NpuModuleMemProcessor::ProcessedDataFormat NpuModuleMemProcessor::FormatData(const OriDataFormat &oriData,
                                                                             uint16_t deviceId,
                                                                             const ProfTimeRecord &timeRecord,
                                                                             SyscntConversionParams &params)
{
    ProcessedDataFormat processedData;
    NpuModuleMemData data;
    if (!Reserve(processedData, oriData.size())) {
        ERROR("Reserve for NpuModuleMem data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.moduleId, data.syscnt, data.totalSize, data.deviceType) = row;
        HPFloat timestamp{GetTimeFromSyscnt(data.syscnt, params)};
        processedData.emplace_back(
            data.moduleId, GetLocalTime(timestamp, timeRecord).Uint64(), data.totalSize, deviceId);
    }
    return processedData;
}

bool NpuModuleMemProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    ProfTimeRecord timeRecord;
    SyscntConversionParams params;
    DBInfo npuModuleMemDB("npu_module_mem.db", "NpuModuleMem");
    bool flag = true;
    MAKE_SHARED0_NO_OPERATION(npuModuleMemDB.database, NpuModuleMemDB);
    auto deviceList = File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    bool timeFlag = Context::GetInstance().GetSyscntConversionParams(params, HOST_ID, fileDir);
    for (const auto &devicePath: deviceList) {
        std::string dbPath = File::PathJoin({devicePath, SQLITE, npuModuleMemDB.dbName});
        // 并不是所有场景都有NpuModuleMem数据
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
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        INFO("Start to process %, deviceId:%.", dbPath, deviceId);
        if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir)) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            flag = false;
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(npuModuleMemDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(npuModuleMemDB);
        if (oriData.empty()) {
            flag = false;
            ERROR("Get % data failed in %.", npuModuleMemDB.tableName, dbPath);
            continue;
        }
        auto processedData = FormatData(oriData, deviceId, timeRecord, params);
        if (!SaveData(processedData, TABLE_NAME_NPU_MODULE_MEM)) {
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