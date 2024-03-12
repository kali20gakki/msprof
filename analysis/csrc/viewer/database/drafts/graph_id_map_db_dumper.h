/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : graph_id_map_db_dumper.h
 * Description        : graph_id_map落盘
 * Author             : msprof team
 * Creation Date      : 2024/3/7
 * *****************************************************************************
 */
#ifndef ANALYSIS_CSRC_VIEWER_DATABASE_GRAPH_ID_MAP_DB_DUMPER_H
#define ANALYSIS_CSRC_VIEWER_DATABASE_GRAPH_ID_MAP_DB_DUMPER_H

#include "analysis/csrc/viewer/database/drafts/base_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using GraphIdMapData = std::vector<std::tuple<std::string, std::string, uint32_t, uint64_t, std::string, uint32_t>>;
class GraphIdMapDBDumper : public BaseDumper<GraphIdMapDBDumper> {
    using GraphIdMaps = std::vector<std::shared_ptr<MsprofAdditionalInfo>>;
public:
    explicit GraphIdMapDBDumper(const std::string &hostFilePath);
    GraphIdMapData GenerateData(const GraphIdMaps &graphIdMaps);
};
} // Drafts
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_CSRC_VIEWER_DATABASE_GRAPH_ID_MAP_DB_DUMPER_H
