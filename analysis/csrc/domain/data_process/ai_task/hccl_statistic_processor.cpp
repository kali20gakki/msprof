/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_statistic_processor.cpp
 * Description        : hccl_statistic_processor，处理HcclOpReport表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/17
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/hccl_statistic_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;
HcclStatisticProcessor::HcclStatisticProcessor(const std::string& profPaths) : DataProcessor(profPaths)
{}
OriHcclDataFormat HcclStatisticProcessor::LoadData(const DBInfo& dbInfo, const std::string& dbPath)
{
    OriHcclDataFormat oriData;
    if (dbInfo.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{
        "SELECT op_type, occurrences,  round(total_time/1000.0, 3), round(min/1000.0, 3), "
        "round(avg/1000.0, 3), round(max/1000.0, 3), round(ratio, 3) FROM " + dbInfo.tableName};
    if (!dbInfo.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
    }
    return oriData;
}

std::vector<HcclStatisticData> HcclStatisticProcessor::FormatData(const OriHcclDataFormat& oriData,
                                                                  const uint16_t deviceId)
{
    std::vector<HcclStatisticData> processedData;
    HcclStatisticData data;
    if (!Reserve(processedData, oriData.size())) {
        ERROR("Reserve for Hccl Statistic data failed.");
        return processedData;
    }
    data.deviceId = deviceId;
    for (auto& row : oriData) {
        std::tie(data.opType, data.count, data.totalTime, data.min, data.avg, data.max, data.ratio) = row;
        processedData.push_back(data);
    }
    return processedData;
}

bool HcclStatisticProcessor::Process(Analysis::Infra::DataInventory& dataInventory)
{
    bool flag = true;
    std::vector<HcclStatisticData> res;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList) {
        DBInfo hcclStatisticsDB("hccl_single_device.db", "HcclOpReport");
        std::string dbPath = File::PathJoin({devicePath, SQLITE, hcclStatisticsDB.dbName});
        auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (deviceId == INVALID_DEVICE_ID) {
            ERROR("the invalid deviceId cannot to be identified.");
            return false;
        }
        if (!hcclStatisticsDB.ConstructDBRunner(dbPath)) {
            flag = false;
            continue;
        }
        auto status = CheckPathAndTable(dbPath, hcclStatisticsDB, false);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        auto oriData = LoadData(hcclStatisticsDB, dbPath);
        if (oriData.empty()) {
            flag = false;
            ERROR("Hccl Statistics original data is empty. DBPath is %", dbPath);
            continue;
        }
        auto formatData = FormatData(oriData, deviceId);
        if (formatData.empty()) {
            ERROR("Hccl Statistics data format failed, DBPath is %", dbPath);
            flag = false;
            continue;
        }
        res.insert(res.end(), formatData.begin(), formatData.end());
    }
    if (!SaveToDataInventory<HcclStatisticData>(std::move(res), dataInventory, PROCESSOR_NAME_COMM_STATISTIC)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_COMM_STATISTIC);
        flag = false;
    }
    return flag;
}

}
}
