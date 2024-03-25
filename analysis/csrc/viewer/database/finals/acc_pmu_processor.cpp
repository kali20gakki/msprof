/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_processor.cpp
 * Description        : acc_pmu_processor，处理AccPmu表数据
 * Author             : msprof team
 * Creation Date      : 2024/2/29
 * *****************************************************************************
 */
#include "analysis/csrc/viewer/database/finals/acc_pmu_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/parser/environment/context.h"
#include "analysis/csrc/dfx/error_code.h"

namespace Analysis {
namespace Viewer {
namespace Database {
using namespace Analysis::Parser::Environment;
using namespace Analysis::Utils;
namespace {
struct AccPmuData {
    uint32_t acc_id = 0;
    uint32_t read_bandwidth = 0;
    uint32_t write_bandwidth = 0;
    uint32_t read_ost = 0;
    uint32_t write_ost = 0;
    double timestamp = 0.0;
};
}

AccPmuProcessor::AccPmuProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths)
    : TableProcessor(msprofDBPath, profPaths) {}

bool AccPmuProcessor::Run()
{
    INFO("AccPmuProcessor Run.");
    bool flag = TableProcessor::Run();
    PrintProcessorResult(flag, PROCESSOR_NAME_ACC_PMU);
    return flag;
}

AccPmuProcessor::OriDataFormat AccPmuProcessor::GetData(const DBInfo &accPmuDB)
{
    OriDataFormat oriData;
    // acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp
    std::string sql{"SELECT acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp "
                    "FROM " + accPmuDB.tableName};
    if (!accPmuDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Failed to obtain data from the % table.", accPmuDB.tableName);
    }
    return oriData;
}

AccPmuProcessor::ProcessedDataFormat AccPmuProcessor::FormatData(const OriDataFormat &oriData,
                                                                 const Utils::ProfTimeRecord &timeRecord,
                                                                 const uint16_t deviceId)
{
    ProcessedDataFormat processedData;
    AccPmuData data;
    if (!Utils::Reserve(processedData, oriData.size())) {
        ERROR("Reserve for AccPmu data failed.");
        return processedData;
    }
    for (auto &row: oriData) {
        std::tie(data.acc_id, data.read_bandwidth, data.write_bandwidth,
                 data.read_ost, data.write_ost, data.timestamp) = row;
        HPFloat timestamp{data.timestamp};
        processedData.emplace_back(
            static_cast<uint16_t>(data.acc_id), data.read_bandwidth, data.write_bandwidth,
            data.read_ost, data.write_ost, GetLocalTime(timestamp, timeRecord).Uint64(), deviceId);
    }
    return processedData;
}

bool AccPmuProcessor::Process(const std::string &fileDir)
{
    INFO("Start to process %.", fileDir);
    Utils::ProfTimeRecord timeRecord;
    DBInfo accPmuDB("acc_pmu.db", "AccPmu");
    bool flag = true;
    MAKE_SHARED0_NO_OPERATION(accPmuDB.database, AccPmuDB);
    bool timeFlag = Context::GetInstance().GetProfTimeRecordInfo(timeRecord, fileDir);
    auto deviceList = Utils::File::GetFilesWithPrefix(fileDir, DEVICE_PREFIX);
    for (const auto& devicePath: deviceList) {
        std::string dbPath = Utils::File::PathJoin({devicePath, SQLITE, accPmuDB.dbName});
        // 校验是否存在AccPmu数据
        auto status = CheckPath(dbPath);
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
        INFO("Start to process device data in %, deviceId:[%].", dbPath, deviceId);
        if (!timeFlag) {
            ERROR("Failed to obtain the time in start_info and end_info.");
            flag = false;
            continue;
        }
        MAKE_SHARED_RETURN_VALUE(accPmuDB.dbRunner, DBRunner, false, dbPath);
        auto oriData = GetData(accPmuDB);
        if (oriData.empty()) {
            ERROR("Get % data failed in %.", accPmuDB.tableName, dbPath);
            flag = false;
            continue;
        }
        auto processedData = FormatData(oriData, timeRecord, deviceId);
        if (!SaveData(processedData, TABLE_NAME_ACC_PMU)) {
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