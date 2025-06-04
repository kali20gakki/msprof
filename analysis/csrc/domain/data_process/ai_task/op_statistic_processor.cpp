/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : op_statistic_processor.cpp
 * Description        : op_statistic_processor，处理OpReport表数据
 * Author             : msprof team
 * Creation Date      : 2025/5/29
 * *****************************************************************************
 */


#include "analysis/csrc/domain/data_process/ai_task/op_statistic_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

OpStatisticProcessor::OpStatisticProcessor(const std::string& profPaths) : DataProcessor(profPaths)
{}
OriOpCountDataFormat OpStatisticProcessor::LoadData(const DBInfo& dbInfo, const std::string& dbPath)
{
    OriOpCountDataFormat oriData;
    if (dbInfo.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql{
        "SELECT op_type, core_type, occurrences,  round(total_time/1000.0, 3), round(min/1000.0, 3), "
        "round(avg/1000.0, 3), round(max/1000.0, 3), round(ratio, 3) FROM " + dbInfo.tableName +
            " WHERE op_type != 'N/A' and core_type not in ('WRITE_BACK', 'INVALID')"
    };
    if (!dbInfo.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", dbInfo.tableName);
    }
    return oriData;
}

std::vector<OpStatisticData> OpStatisticProcessor::FormatData(const OriOpCountDataFormat& oriData,
                                                              const uint16_t deviceId)
{
    std::vector<OpStatisticData> processedData;
    OpStatisticData data;
    if (!Reserve(processedData, oriData.size())) {
        ERROR("Reserve for Op Statistic data failed.");
        return processedData;
    }
    data.deviceId = deviceId;
    for (auto& row : oriData) {
        std::tie(data.opType, data.coreType, data.count, data.totalTime, data.min, data.avg, data.max,
                 data.ratio) = row;
        processedData.push_back(data);
    }
    return processedData;
}

bool OpStatisticProcessor::Process(Analysis::Infra::DataInventory& dataInventory)
{
    bool flag = true;
    std::vector<OpStatisticData> res;
    auto deviceList = File::GetFilesWithPrefix(profPath_, DEVICE_PREFIX);
    for (const auto& devicePath : deviceList) {
        DBInfo opCounterDB("op_counter.db", "op_report");
        std::string dbPath = File::PathJoin({devicePath, SQLITE, opCounterDB.dbName});
        auto deviceId = Utils::GetDeviceIdByDevicePath(devicePath);
        if (deviceId == INVALID_DEVICE_ID) {
            ERROR("the invalid deviceId cannot to be identified, profPath is %", profPath_);
            return false;
        }
        if (!opCounterDB.ConstructDBRunner(dbPath)) {
            flag = false;
            continue;
        }
        auto status = CheckPathAndTable(dbPath, opCounterDB);
        if (status != CHECK_SUCCESS) {
            if (status == CHECK_FAILED) {
                flag = false;
            }
            continue;
        }
        auto oriData = LoadData(opCounterDB, dbPath);
        if (oriData.empty()) {
            flag = false;
            ERROR("Op Statistics original data is empty. DBPath is %", dbPath);
            continue;
        }
        auto formatData = FormatData(oriData, deviceId);
        if (formatData.empty()) {
            ERROR("Op Statistics data format failed, DBPath is %", dbPath);
            flag = false;
            continue;
        }
        res.insert(res.end(), formatData.begin(), formatData.end());
    }
    if (!SaveToDataInventory<OpStatisticData>(std::move(res), dataInventory, PROCESSOR_NAME_OP_STATISTIC)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_OP_STATISTIC);
        flag = false;
    }
    return flag;
}
}
}