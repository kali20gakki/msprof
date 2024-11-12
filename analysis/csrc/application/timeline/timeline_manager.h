/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : timeline_manager.h
 * Description        : timeline_manager，timeline导出调度
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_TIMELINE_MANAGER_H
#define ANALYSIS_APPLICATION_TIMELINE_MANAGER_H

#include <string>
#include <utility>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/infrastructure/dump_tools/include/dump_tool.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
class TimelineManager {
public:
    explicit TimelineManager(const std::string &profPath, const std::string &outputPath)
        : profPath_(profPath), outputPath_(outputPath) {};
    bool Run(DataInventory &dataInventory);
private:
    bool ProcessTimeLine(DataInventory &dataInventory);
    bool PreDumpJson(DataInventory &dataInventory);
    void PostDumpJson();
    void WriteFile(const std::string &filePrefix, FileCategory category, const char *content);
private:
    std::string profPath_;
    std::string outputPath_;
    std::unordered_map<FileCategory, std::string> fileType_;
};
}
}
#endif // ANALYSIS_APPLICATION_TIMELINE_MANAGER_H
