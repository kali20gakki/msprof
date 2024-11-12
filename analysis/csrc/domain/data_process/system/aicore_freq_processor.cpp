/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : aicore_freq_processor.cpp
 * Description        : aicore_freq_processor，处理aicore freq表 低功耗变频相关数据
 * Author             : msprof team
 * Creation Date      : 2024/07/30
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/system/aicore_freq_processor.h"
#include <algorithm>
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;

AicoreFreqProcessor::AicoreFreqProcessor(const std::string& profPath) : DataProcessor(profPath)
{}

bool AicoreFreqProcessor::Process(DataInventory& dataInventory)
{
    auto version = Context::GetInstance().GetPlatformVersion(Parser::Environment::DEFAULT_DEVICE_ID, profPath_);
    if (version != static_cast<int>(Chip::CHIP_V4_1_0) && version != static_cast<int>(Chip::CHIP_V1_1_1)) {
        // 当前仅支持910b 和 310b处理低功耗变频数据
        WARN("This platformVersion: % does not support the processing of aicore freq data.", version);
        return true;
    }
    ProfTimeRecord timeRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, profPath_)) {
        ERROR("Failed to GetProfTimeRecordInfo, fileDir is %.", profPath_);
        return false;
    }
    bool flag = true;
    std::vector<AicoreFreqData> res;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList) {
        auto deviceId = GetDeviceIdByDevicePath(devicePath);
        if (deviceId == Parser::Environment::HOST_ID) {
            ERROR("The invalid deviceId cannot to be identified.");
            flag = false;
            continue;
        }
        Utils::SyscntConversionParams params;
        if (!Context::GetInstance().GetSyscntConversionParams(params, deviceId, profPath_)) {
            ERROR("GetSyscntConversionParams failed, profPath is %.", profPath_);
            flag = false;
            continue;
        }
        OriFreqDataFormat freqData;
        if (!ProcessData(devicePath, freqData)) {
            ERROR("Get original data failed.");
            flag = false;
            continue;
        }
        std::vector<AicoreFreqData> processedData;
        if (!FormatData(deviceId, timeRecord, params, freqData, processedData)) {
            ERROR("FormatData failed, fileDir is %.", devicePath);
            flag = false;
            continue;
        }
        std::sort(processedData.begin(), processedData.end(), [](const AicoreFreqData &ld, const AicoreFreqData &rd) {
            return ld.timestamp < rd.timestamp;
        });
        res.insert(res.end(), processedData.begin(), processedData.end());
    }
    if (!SaveToDataInventory<AicoreFreqData>(std::move(res), dataInventory, PROCESSOR_NAME_AICORE_FREQ)) {
        flag = false;
        ERROR("Save data failed, %.", PROCESSOR_NAME_AICORE_FREQ);
    }
    return flag;
}

bool AicoreFreqProcessor::ProcessData(const std::string& devicePath, OriFreqDataFormat& oriData)
{
    DBInfo dbInfo("freq.db", "FreqParse");
    std::string dbPath = File::PathJoin({devicePath, SQLITE, dbInfo.dbName});
    if (!dbInfo.ConstructDBRunner(dbPath)) {
        return false;
    }
    auto status = CheckPathAndTable(dbPath, dbInfo);
    if (status == CHECK_FAILED) {
        return false;
    } else if (status == CHECK_SUCCESS) {
        oriData = LoadData(dbPath, dbInfo);
    }
    if (oriData.empty()) {
        WARN("Original data is empty. DBPath is %", dbPath);
    }
    return true;
}

OriFreqDataFormat AicoreFreqProcessor::LoadData(const std::string& dbPath, DBInfo& freqDB)
{
    INFO("AicoreFreqProcessor GetData, dir is %", dbPath);
    OriFreqDataFormat oriData;
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

bool AicoreFreqProcessor::FormatData(const uint16_t& deviceId, const ProfTimeRecord& timeRecord,
                                     const Utils::SyscntConversionParams& params,
                                     const OriFreqDataFormat& freqData,
                                     std::vector<AicoreFreqData>& processedData)
{
    INFO("AicoreFreqProcessor FormatData.");
    double freq = 0.0;
    if (!Context::GetInstance().GetPmuFreq(freq, deviceId, profPath_)) {
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
    AicoreFreqData aicoreFreqData;
    // 将频率置为默认频率，避免最后插入结束记录时 频率为0
    aicoreFreqData.freq = freq;
    aicoreFreqData.deviceId = deviceId;
    for (auto& row : freqData) {
        uint64_t devSyscnt;
        std::tie(devSyscnt, aicoreFreqData.freq) = row;
        HPFloat hostTimestamp{GetTimeFromSyscnt(devSyscnt, params)};
        aicoreFreqData.timestamp = GetLocalTime(hostTimestamp, timeRecord).Uint64();
        if (aicoreFreqData.timestamp < timeRecord.startTimeNs) {
            freq = aicoreFreqData.freq;
            continue;
        }
        processedData.push_back(aicoreFreqData);
    }
    aicoreFreqData.timestamp = timeRecord.endTimeNs;
    // 结束时间的freq为最后一条记录的freq
    processedData.push_back(aicoreFreqData);
    // 如果开始时间前无记录，则使用默认freq；如果有记录，则使用开始时间前的freq(此时添加 db里的数据将为乱序，但不影响数据读取)
    aicoreFreqData.timestamp = timeRecord.startTimeNs;
    aicoreFreqData.freq = freq;
    processedData.push_back(aicoreFreqData);
    return true;
}
}
}