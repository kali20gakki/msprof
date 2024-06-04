/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : acc_pmu_persistence.cpp
 * Description        : acc pmu数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/6/4
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/acc_pmu_persistence.h"
#include "analysis/csrc/domain/entities/hal/include/hal_log.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/domain/services/persistence/persistence_utils.h"
#include "analysis/csrc/domain/services/parser/log/include/stars_soc_parser.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {
namespace Domain {
using namespace Viewer::Database;
using namespace Utils;
const int READ = 0;
const int WRITE = 1;
// acc_id, read_bandwidth, write_bandwidth, read_ost, write_ost, timestamp
using ProcessedDataFormat = std::vector<std::tuple<uint16_t, uint64_t, uint64_t, uint64_t, uint64_t, double>>;

ProcessedDataFormat GenerateAccPmuData(std::vector<HalLogData>& logData, const DeviceContext& context)
{
    auto params = GenerateSyscntConversionParams(context);
    ProcessedDataFormat processedData;
    for (auto& data : logData) {
        if (data.type != ACC_PMU) {
            continue;
        }
        auto hfFloatTime = GetTimeFromSyscnt(data.accPmu.timestamp, params);
        processedData.emplace_back(data.accPmu.accId, data.accPmu.bandwidth[READ], data.accPmu.bandwidth[WRITE],
                                   data.accPmu.ost[READ], data.accPmu.ost[WRITE], hfFloatTime.Double());
    }
    return processedData;
}

uint32_t AccPmuPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto accPmu = dataInventory.GetPtr<std::vector<HalLogData>>();
    DBInfo accPmuDB("acc_pmu.db", "AccPmu");
    MAKE_SHARED0_RETURN_VALUE(accPmuDB.database, AccPmuDB, ANALYSIS_ERROR);
    std::string dbPath = File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, accPmuDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(accPmuDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    auto data = GenerateAccPmuData(*accPmu, deviceContext);
    if (data.empty()) {
        INFO("There is no acc pmu data don't need to persistence!");
        return ANALYSIS_OK;
    }
    auto res = SaveData(data, accPmuDB, dbPath);
    if (res) {
        INFO("Process % done!", accPmuDB.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(AccPmuPersistence, true, StarsSocParser);
REGISTER_PROCESS_DEPENDENT_DATA(AccPmuPersistence, std::vector<HalLogData>);
REGISTER_PROCESS_SUPPORT_CHIP(AccPmuPersistence, CHIP_V4_1_0);
}
}