/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : flip_task_db_dumper.h
 * Description        : FlipTaskDBDumper
 * Author             : msprof team
 * Creation Date      : 2024/3/6
 * *****************************************************************************
 */


#ifndef ANALYSIS_VIEWER_DATABASE_FLIP_TASK_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_FLIP_TASK_DB_DUMPER_H

#include <thread>
#include <unordered_map>
#include "analysis/csrc/viewer/database/db_runner.h"
#include "analysis/csrc/viewer/database/database.h"
#include "analysis/csrc/parser/adapter/flip.h"
#include "analysis/csrc/viewer/database/drafts/base_dumper.h"

namespace Analysis {
namespace Viewer {
namespace Database {
namespace Drafts {
using FlipTaskData = std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint32_t>>;
class FlipTaskDBDumper final : public BaseDumper<FlipTaskDBDumper> {
    using FlipTask = Analysis::Parser::Adapter::FlipTask;
    using FlipTasks = std::vector<std::shared_ptr<FlipTask>>;
public:
    explicit FlipTaskDBDumper(const std::string& hostFilePath);
    FlipTaskData GenerateData(const FlipTasks &flipTasks);
};
} // Drafts
} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_FLIP_TASK_DB_DUMPER_H
