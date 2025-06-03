/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : model_name_processor.cpp
 * Description        : model_name_processor，处理ModelName表数据
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#include "analysis/csrc/domain/data_process/ai_task/model_name_processor.h"
#include "analysis/csrc/domain/services/environment/context.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Domain::Environment;
using namespace Analysis::Utils;

ModelNameProcessor::ModelNameProcessor(const std::string &profPath) : DataProcessor(profPath) {}

bool ModelNameProcessor::Process(DataInventory &dataInventory)
{
    DBInfo modelNameDB("ge_model_info.db", "ModelName");

    std::string dbPath = Utils::File::PathJoin({profPath_, HOST, SQLITE, modelNameDB.dbName});
    if (!modelNameDB.ConstructDBRunner(dbPath)) {
        return false;
    }

    auto status = CheckPathAndTable(dbPath, modelNameDB, false);
    if (status == CHECK_FAILED) {
        return false;
    } else if (status == NOT_EXIST) {
        return true;
    }

    auto modelNameData = LoadData(modelNameDB, dbPath);
    if (modelNameData.empty()) {
        ERROR("model name original data is empty. DBPath is %", dbPath);
        return false;
    }

    auto processedData = FormatData(modelNameData);
    if (processedData.empty()) {
        ERROR("format model name data error");
        return false;
    }

    if (!SaveToDataInventory<ModelName>(std::move(processedData), dataInventory, PROCESSOR_NAME_MODEL_NAME)) {
        ERROR("Save data failed, %.", PROCESSOR_NAME_MODEL_NAME);
        return false;
    }
    return true;
}


OriModelNameDataFormat ModelNameProcessor::LoadData(const DBInfo &modelNameDB, const std::string &dbPath)
{
    OriModelNameDataFormat oriData;
    if (modelNameDB.dbRunner == nullptr) {
        ERROR("Create % connection failed.", dbPath);
        return oriData;
    }
    std::string sql = "SELECT model_id, model_name FROM " + modelNameDB.tableName;
    if (!modelNameDB.dbRunner->QueryData(sql, oriData)) {
        ERROR("Query model name failed, db path is %.", dbPath);
        return oriData;
    }
    return oriData;
}

std::vector<ModelName> ModelNameProcessor::FormatData(const OriModelNameDataFormat &oriData)
{
    std::vector<ModelName> formatData;
    if (!Reserve(formatData, oriData.size())) {
        ERROR("Reserved for model name data failed.");
        return formatData;
    }
    ModelName data;
    for (const auto& row : oriData) {
        std::tie(data.modelId, data.modelName) = row;
        formatData.push_back(data);
    }
    return formatData;
}
}
}