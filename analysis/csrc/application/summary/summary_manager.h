/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : summary_manager.h
 * Description        : summary_manager，summary导出调度
 * Author             : msprof team
 * Creation Date      : 2025/5/23
 * *****************************************************************************
 */

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
    static std::vector<std::string> GetProcessList();
private:
    bool ProcessSummary(DataInventory& dataInventory);
private:
    std::string profPath_;
    std::string outputPath_;
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_MANAGER_H
