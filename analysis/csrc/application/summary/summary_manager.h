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

#ifndef ANALYSIS_APPLICATION_SUMMARY_MANAGER_H
#define ANALYSIS_APPLICATION_SUMMARY_MANAGER_H

#include <string>
#include <utility>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;

class SummaryManager {
public:
    explicit SummaryManager(const std::string& profPath, const std::string& outputPath)
        : profPath_(profPath), outputPath_(outputPath) {};
    bool Run(DataInventory& dataInventory);
    const static std::set<std::string>& GetProcessList();
private:
    bool ProcessSummary(DataInventory& dataInventory);
private:
    std::string profPath_;
    std::string outputPath_;
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_MANAGER_H
