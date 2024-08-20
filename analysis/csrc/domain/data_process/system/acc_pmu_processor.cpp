/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_processor.cpp
 * Description        : acc_pmu_processor，处理AccPmu表数据
 * Author             : msprof team
 * Creation Date      : 2024/7/20
 * *****************************************************************************
 */

#include "acc_pmu_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
AccPmuProcessor::AccPmuProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool AccPmuProcessor::Process(DataInventory &dataInventory)
{
    Utils::ProfTimeRecord timeRecord;
    if (!Context::GetInstance().GetProfTimeRecordInfo(timeRecord, profPath_)) {
        ERROR("Failed to obtain the time in start_info and end_info.");
        return false;
    }
    bool flag = true;
    auto deviceList = Utils::File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    std::vector<AccPmuData> res;
    for (const auto& devicePath: deviceList) {
        DBInfo accPmuDB("acc_pmu.db", "AccPmu");
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, accPmuDB.dbName});
        if (!accPmuDB.ConstructDBRunner(dbPath)) {
            flag = false;
            continue;
        }
        auto status = CheckPathAndTable(dbPath, accPmuDB);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        uint16_t deviceId = GetDeviceIdByDevicePath(devicePath);
        if (deviceId == Parser::Environment::HOST_ID) {
            ERROR("the invalid deviceId cannot to be identified.");
            flag = false;
            continue;
        }
        auto oriData = LoadData(accPmuDB, dbPath);
        if (oriData.empty()) {
            ERROR("Get acc_pmu original data failed in %.", dbPath);
            flag = false;
            continue;
        }
        auto processedData = FormatData(oriData, timeRecord, deviceId);
        if (processedData.empty()) {
            ERROR("format acc_pmu data failed, dbPath is ", dbPath);
            flag = false;
            continue;
        }
        res.insert(res.end(), processedData.begin(), processedData.end());
    }
    if (!SaveToDataInventory<AccPmuData>(std::move(res), dataInventory, PROCESSOR_NAME_ACC_PMU)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_ACC_PMU);
        flag = false;
    }
    return flag;
}

OriAccPmuData AccPmuProcessor::LoadData(const DBInfo &accPmuDB, const std::string &dbPath)
{
    OriAccPmuData oriData;
    if (accPmuDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{"SELECT acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp "
                    "FROM " + accPmuDB.tableName};
    if (!accPmuDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", accPmuDB.tableName);
    }
    return oriData;
}

std::vector<AccPmuData> AccPmuProcessor::FormatData(
    const OriAccPmuData &oriData, const Utils::ProfTimeRecord &timeRecord, const uint16_t deviceId)
{
    std::vector<AccPmuData> processedData;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for AccPmu data failed.");
        return processedData;
    }
    AccPmuData data;
    uint32_t accId;
    double timestamp;
    data.deviceId = deviceId;
    for (const auto &row: oriData) {
        std::tie(accId, data.readBwLevel, data.writeBwLevel, data.readOstLevel, data.writeOstLevel, timestamp) = row;
        HPFloat time{timestamp};
        data.accId = static_cast<uint16_t>(accId);
        data.timestampNs = GetLocalTime(time, timeRecord).Uint64();
        processedData.push_back(data);
    }
    return processedData;
}
}
}
