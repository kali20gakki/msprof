/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/


#ifndef ANALYSIS_VIEWER_DATABASE_FLIP_TASK_DB_DUMPER_H
#define ANALYSIS_VIEWER_DATABASE_FLIP_TASK_DB_DUMPER_H

#include <thread>
#include <unordered_map>
#include "analysis/csrc/infrastructure/db/include/db_runner.h"
#include "analysis/csrc/infrastructure/db/include/database.h"
#include "analysis/csrc/domain/services//adapter/flip.h"
#include "analysis/csrc/domain/services/persistence/host/base_dumper.h"

namespace Analysis {
namespace Domain {
using FlipTaskData = std::vector<std::tuple<uint32_t, uint64_t, uint32_t, uint32_t>>;
class FlipTaskDBDumper final : public BaseDumper<FlipTaskDBDumper> {
    using FlipTask = Analysis::Domain::Adapter::FlipTask;
    using FlipTasks = std::vector<std::shared_ptr<FlipTask>>;
public:
    explicit FlipTaskDBDumper(const std::string& hostFilePath);
    FlipTaskData GenerateData(const FlipTasks &flipTasks);
};
} // Domain
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_FLIP_TASK_DB_DUMPER_H
