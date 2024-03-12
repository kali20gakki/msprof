/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : graph_id_map_db_dumper.cpp
 * Description        : graph_id_map落盘
 * Author             : msprof team
 * Creation Date      : 2024/3/7
 * *****************************************************************************
 */
#include "collector/inc/toolchain/prof_common.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/drafts/number_mapping.h"
#include "analysis/csrc/parser/host/cann/hash_data.h"
#include "analysis/csrc/parser/host/cann/type_data.h"
#include "analysis/csrc/viewer/database/drafts/graph_id_map_db_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using HashData = Analysis::Parser::Host::Cann::HashData;
using TypeData = Analysis::Parser::Host::Cann::TypeData;
GraphIdMapDBDumper::GraphIdMapDBDumper(const std::string &hostFilePath)
    : BaseDumper<GraphIdMapDBDumper>(hostFilePath, "GraphIdMap")
{
    MAKE_SHARED0_NO_OPERATION(database_, GraphIdMapDB);
}

GraphIdMapData GraphIdMapDBDumper::GenerateData(const GraphIdMaps &graphIdMaps)
{
    GraphIdMapData data;
    if (Utils::Reserve(data, graphIdMaps.size())) {
        for (auto &graphIdMap: graphIdMaps) {
            auto idMapStruct = Utils::ReinterpretConvert<MsprofGraphIdInfo *>(graphIdMap->data);

            auto level = NumberMapping::Get(NumberMapping::MappingType::LEVEL, graphIdMap->level);
            auto structType = TypeData::GetInstance().Get(graphIdMap->level, graphIdMap->type);
            auto threadId = graphIdMap->threadId;
            auto timestamp = graphIdMap->timeStamp;
            auto modelName = HashData::GetInstance().Get(idMapStruct->modelName);
            auto graphId = idMapStruct->graphId;
            data.emplace_back(level, structType, threadId, timestamp, modelName, graphId);
        }
    }
    return data;
}
} // Drafts
} // Database
} // Viewer
} // Analysis
