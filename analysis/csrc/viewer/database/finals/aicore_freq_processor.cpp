/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_processor.h
 * Description        : aicore_freq_processor，处理aicore freq表 低功耗变频相关数据
 * Author             : msprof team
 * Creation Date      : 2024/05/17
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/aicore_freq_processor.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;

namespace {
struct FreqData {
    uint64_t devSyscnt = UINT64_MAX;
    double freq = 0.0;
};
}

AicoreFreqProcessor::AicoreFreqProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool AicoreFreqProcessor::Run()
{
    INFO("AicoreFreqProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_AICORE_FREQ);
    return flag;
}

bool AicoreFreqProcessor::Process(const std::string &fileDir)
{
    INFO("AicoreFreqProcessor Process, dir is %", fileDir);
    auto version = Context::GetInstance().GetPlatformVersion(Parser::Environment::DEFAULT_DEVICE_ID, fileDir);
    if (version != static_cast<int>(Chip::CHIP_V4_1_0) && version != static_cast<int>(Chip::CHIP_V1_1_1)) {
        // 当前仅支持910b 和 310b处理低功耗变频数据
        WARN("This platformVersion: % does not support the processing of aicore freq data.", version);
        return true;
    }
    ThreadData threadData;
    DBInfo freqDB("freq.db", "FreqParse");
    bool flag = true;
    auto deviceList = File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = File::PathJoin({devicePath, SQLITE, freqDB.dbName});
        // 仅db校验失败时返回，如果无db继续流程
        auto status = CheckPath(dbPath);
        if (status == CHECK_FAILED) {
            flag = false;
            continue;
        }
        threadData.deviceId = GetDeviceIdByDevicePath(devicePath);
        if (!Context::GetInstance().GetProfTimeRecordInfo(threadData.timeRecord, fileDir, threadData.deviceId)) {
            ERROR("Failed to GetProfTimeRecordInfo, fileDir is %, device id is %.", fileDir, threadData.deviceId);
            return false;
        }
        Utils::SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, threadData.deviceId, fileDir)) {
            ERROR("GetSyscntConversionParams failed, profPath is %.", fileDir);
            flag = false;
            continue;
        }
        OriDataFormat freqData;
        if (status == CHECK_SUCCESS) {
            freqData = GetData(dbPath, freqDB);
        }
        ProcessedDataFormat processedData;
        if (!FormatData(fileDir, threadData, params, freqData, processedData)) {
            ERROR("FormatData failed, fileDir is %.", fileDir);
            flag = false;
            continue;
        }
        if (!SaveData(processedData, TABLE_NAME_AICORE_FREQ)) {
            flag = false;
            ERROR("Save data failed, %.", dbPath);
            continue;
        }
    }
    return flag;
}

AicoreFreqProcessor::OriDataFormat AicoreFreqProcessor::GetData(const std::string &dbPath, DBInfo &freqDB)
{
    INFO("AicoreFreqProcessor GetData, dir is %", dbPath);
    OriDataFormat oriData;
    MAKE_SHARED_RETURN_VALUE(freqDB.dbRunner, DBRunner, oriData, dbPath);
    if (freqDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT syscnt, freq FROM " + freqDB.tableName;
    if (!freqDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query freq data failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

bool AicoreFreqProcessor::FormatData(const std::string fileDir, const ThreadData &threadData,
                                     const Utils::SyscntConversionParams &params,
                                     const OriDataFormat &freqData, ProcessedDataFormat &processedData)
{
    INFO("AicoreFreqProcessor FormatData.");
    double freq = 0.0;
    if (!Context::GetInstance().GetPmuFreq(freq, threadData.deviceId, fileDir)) {
        ERROR("Get default freq data failed.");
        return false;
    }
    if (freqData.empty()) {
        WARN("freq original data is empty, it will use default freq.");
    }
    // 2是开始和结束的两条记录
    if (!Reserve(processedData, freqData.size() + 2)) {
        ERROR("Reserve for freq data failed.");
        return false;
    }
    FreqData tempData;
    // 将频率置为默认频率，避免最后插入结束记录时 频率为0
    tempData.freq = freq;
    for (auto &row: freqData) {
        std::tie(tempData.devSyscnt, tempData.freq) = row;
        HPFloat hostTimestamp{GetTimeFromSyscnt(tempData.devSyscnt, params)};
        uint64_t timestamp = GetLocalTime(hostTimestamp, threadData.timeRecord).Uint64();
        if (timestamp < threadData.timeRecord.startTimeNs) {
            freq = tempData.freq;
            continue;
        }
        processedData.emplace_back(threadData.deviceId, timestamp, tempData.freq);
    }
    // 如果开始时间前无记录，则使用默认freq；如果有记录，则使用开始时间前的freq(此时添加 db里的数据将为乱序，但不影响数据读取)
    processedData.emplace_back(threadData.deviceId, threadData.timeRecord.startTimeNs, freq);
    // 结束时间的freq为最后一条记录的freq. 如果endTime为DEFAULT_END_TIME_US * MILLI_SECOND,说明无end_info,不使用非法值
    if (threadData.timeRecord.endTimeNs != DEFAULT_END_TIME_NS) {
        processedData.emplace_back(threadData.deviceId, threadData.timeRecord.endTimeNs, tempData.freq);
    }
    return true;
}

}
}
}