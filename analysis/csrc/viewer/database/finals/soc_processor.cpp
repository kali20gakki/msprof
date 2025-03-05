/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : soc_processor.cpp
 * Description        : soc_processor，处理InterSoc表数据
 * Author             : msprof team
 * Creation Date      : 2024/3/1
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/soc_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
namespace {
struct SocBandwidthLevelData {
    uint32_t l2BufferBwLevel = 0;
    uint32_t mataBwLevel = 0;
    double timestamp = 0.0;
};
}

SocProcessor::SocProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool SocProcessor::Run()
{
    INFO("SocProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_SOC);
    return flag;
}

SocProcessor::OriDataFormat SocProcessor::GetData(const DBInfo &socBandwidthLevelDB)
{
    OriDataFormat oriData;
    // l2_buffer_bw_level, mata_bw_level, sys_time
    std::string sql{"SELECT l2_buffer_bw_level, mata_bw_level, sys_time "
                    "FROM " + socBandwidthLevelDB.tableName};
    if (!socBandwidthLevelDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", socBandwidthLevelDB.tableName);
    }
    return oriData;
}

SocProcessor::ProcessedDataFormat SocProcessor::FormatData(const OriDataFormat &oriData,
                                                           const Utils::ProfTimeRecord &timeRecord,
                                                           const uint16_t deviceId)
{
    ProcessedDataFormat processedData;
    SocBandwidthLevelData data;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for SocBandwidthLevelData data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.l2BufferBwLevel, data.mataBwLevel, data.timestamp) = row;
        HPFloat timestamp{data.timestamp};
        processedData.emplace_back(
            data.l2BufferBwLevel, data.mataBwLevel, GetLocalTime(timestamp, timeRecord).Uint64(), deviceId);
    }
    return processedData;
}

bool SocProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    Utils::ProfTimeRecord timeRecord;
    DBInfo socProfilerDB("soc_profiler.db", "InterSoc");
    bool flag = true;
    MAKE_SHARED0_NO_OPERATION(socProfilerDB.database, SocProfilerDB);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, socProfilerDB.dbName});
        // 校验是否存在InterSoc数据
        auto status = CheckPath(dbPath);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        if (deviceId == INVALID_DEVICE_ID) {
            ERROR("the invalid deviceId cannot to be identified.");
            flag = false;
            continue;
        }
        INFO("Start to process device data in %, deviceId:[%].", dbPath, deviceId);
        if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir, deviceId)) {
            ERROR("Failed to obtain the time in start_info and end_info. Path is %, device id is %", fileDir, deviceId);
            flag = false;
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(socProfilerDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(socProfilerDB);
        if (oriData.empty()) {
            ERROR("Get % data failed in %.", socProfilerDB.tableName, dbPath);
            flag = false;
            continue;
        }
        auto processedData = FormatData(oriData, timeRecord, deviceId);
        if (!SaveData(processedData, TABLE_NAME_SOC)) {
            ERROR("Save data failed, %.", dbPath);
            flag = false;
            continue;
        }
    }
    INFO("process % ends.", fileDir);
    return flag;
}

}
}
}