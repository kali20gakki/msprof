/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : freq_persistence.cpp
 * Description        : freq数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/6/4
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/device/freq_persistence.h"
#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/domain/services/persistence/device/persistence_utils.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"
#include "analysis/csrc/infrastructure/process/include/process_register.h"

namespace Analysis {
namespace Domain {
using namespace Viewer::Database;
using namespace Utils;
using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint32_t>>;

ProcessedDataFormat FreqPersistence::GenerateFreqData(std::vector<HalFreqLpmData>& freqData)
{
    ProcessedDataFormat processedData;
    for (auto& data : freqData) {
        processedData.emplace_back(data.sysCnt, data.freq);
    }
    return processedData;
}

uint32_t FreqPersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const DeviceContext& deviceContext = static_cast<const DeviceContext&>(context);
    auto freqData = dataInventory.GetPtr<std::vector<HalFreqLpmData>>();
    if (freqData == nullptr) {
        ERROR("There is no freq data, don't need to persistence");
        return ANALYSIS_ERROR;
    }
    DBInfo freqDB("freq.db", "FreqParse");
    MAKE_SHARED0_RETURN_VALUE(freqDB.database, FreqDB, ANALYSIS_ERROR);
    std::string dbPath = Utils::File::PathJoin({deviceContext.GetDeviceFilePath(), SQLITE, freqDB.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(freqDB.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    auto data = GenerateFreqData(*freqData);
    if (data.empty()) {
        INFO("The freq is default data no need to persistence!");
        return ANALYSIS_OK;
    }
    auto res = SaveData(data, freqDB, dbPath);
    if (res) {
        INFO("Process % done!", freqDB.tableName);
        return ANALYSIS_OK;
    }
    ERROR("Save % data failed: %", dbPath);
    return ANALYSIS_ERROR;
}
REGISTER_PROCESS_SEQUENCE(FreqPersistence, true, FreqParser);
REGISTER_PROCESS_DEPENDENT_DATA(FreqPersistence, std::vector<HalFreqLpmData>);
REGISTER_PROCESS_SUPPORT_CHIP(FreqPersistence, CHIP_V4_1_0, CHIP_V1_1_1);
}
}

