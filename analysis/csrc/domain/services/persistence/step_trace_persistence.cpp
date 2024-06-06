/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : step_trace_persistence.cpp
 * Description        : step_trace数据落盘
 * Author             : msprof team
 * Creation Date      : 2024/5/23
 * *****************************************************************************
 */

#include "analysis/csrc/domain/services/persistence/step_trace_persistence.h"
#include <algorithm>
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/domain/services/constant/default_value_constant.h"
#include "analysis/csrc/domain/services/persistence/persistence_utils.h"
#include "analysis/csrc/domain/services/modeling/step_trace/include/step_trace_process.h"
#include "analysis/csrc/infrastructure/resource/chip_id.h"

namespace Analysis {
namespace Domain {
using namespace Viewer::Database;
namespace {
const uint16_t STEP_START_TAG = 60000;
const uint16_t STEP_END_TAG = 60001;
}

StepTraceDataVectorFormat GenerateStepTraceData(std::map<uint32_t, std::vector<StepTraceTasks>>& stepTraceTask)
{
    StepTraceDataVectorFormat processedData;
    for (auto &it : stepTraceTask) {
        auto model_id = it.first;
        uint32_t iter_id = 0;
        for (const auto& element : it.second) {
            processedData.emplace_back(element.indexId, model_id, element.stepTrace.start,
                                       element.stepTrace.end, iter_id);
            iter_id++;
        }
    }
    return processedData;
}

uint32_t ProcessStepTraceDataEntry(DataInventory& dataInventory, const DeviceContext& context, DBInfo& stepTraceDB)
{
    auto stepTraceTask = dataInventory.GetPtr<std::map<uint32_t, std::vector<StepTraceTasks>>>();
    std::string dbPath = Utils::GetDBPath({context.GetDeviceFilePath(), SQLITE, stepTraceDB.dbName});
    auto data = GenerateStepTraceData(*stepTraceTask);
    auto res = SaveData(data, stepTraceDB, dbPath);
    if (!res) {
        ERROR("Process % failed!", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    INFO("Process % done!", stepTraceDB.tableName);
    return ANALYSIS_OK;
}

uint32_t ProcessStepTimeEntry(DataInventory& dataInventory, const DeviceContext& context, DBInfo& stepTraceDB)
{
    auto halTrackDatas = dataInventory.GetPtr<std::vector<HalTrackData>>();
    std::string dbPath = Utils::GetDBPath({context.GetDeviceFilePath(), SQLITE, stepTraceDB.dbName});
    auto data = GenerateStepTime(*halTrackDatas);
    auto res = SaveData(data, stepTraceDB, dbPath);
    if (!res) {
        ERROR("Process % failed!", stepTraceDB.tableName);
        return ANALYSIS_ERROR;
    }
    INFO("Process % done!", stepTraceDB.tableName);
    return ANALYSIS_OK;
}

uint32_t StepTracePersistence::ProcessEntry(DataInventory& dataInventory, const Context& context)
{
    const auto& deviceContext = dynamic_cast<const DeviceContext&>(context);
    DBInfo stepTraceInfo("step_trace.db", "step_trace_data");
    MAKE_SHARED0_NO_OPERATION(stepTraceInfo.database, StepTraceDB);
    std::string dbPath = Utils::GetDBPath({deviceContext.GetDeviceFilePath(), SQLITE, stepTraceInfo.dbName});
    INFO("Start to process %.", dbPath);
    MAKE_SHARED_RETURN_VALUE(stepTraceInfo.dbRunner, DBRunner, ANALYSIS_ERROR, dbPath);
    bool res = ANALYSIS_OK;
    res |= ProcessStepTraceDataEntry(dataInventory, deviceContext, stepTraceInfo);
    stepTraceInfo.tableName = "StepTime";
    res |= ProcessStepTimeEntry(dataInventory, deviceContext, stepTraceInfo);
    return res;
}
}
}