/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : model_name_db_dumper.cpp
 * Description        : modelname落盘
 * Author             : msprof team
 * Creation Date      : 2024/3/7
 * *****************************************************************************
 */
#include "analysis/csrc/domain/services/persistence/host/model_name_db_dumper.h"
#include "analysis/csrc/domain/services/persistence/host/number_mapping.h"
#include "analysis/csrc/domain/services/parser/host/cann/hash_data.h"


namespace Analysis {
namespace Domain {
using HashData = Analysis::Domain::Host::Cann::HashData;
ModelNameDBDumper::ModelNameDBDumper(const std::string &hostFilePath)
    : BaseDumper<ModelNameDBDumper>(hostFilePath, "ModelName")
{
    MAKE_SHARED0_NO_OPERATION(database_, GeModelInfoDB);
}

ModelNameData ModelNameDBDumper::GenerateData(const GraphIdMaps &graphIdMaps)
{
    ModelNameData data;
    if (Utils::Reserve(data, graphIdMaps.size())) {
        for (auto &graphIdMap: graphIdMaps) {
            auto idMapStruct = Utils::ReinterpretConvert<MsprofGraphIdInfo *>(graphIdMap->data);
            auto modelId = idMapStruct->modelId;
            auto modelName = (idMapStruct->modelName == 0) ? "0" :
                HashData::GetInstance().Get(idMapStruct->modelName);
            data.emplace_back(modelId, modelName);
        }
    }
    return data;
}
} // Viewer
} // Analysis
