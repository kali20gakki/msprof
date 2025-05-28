/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : export_manager.h
 * Description        : export模块统一控制类
 * Author             : msprof team
 * Creation Date      : 2024/11/1
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_EXPORT_MANAGER_H
#define ANALYSIS_APPLICATION_EXPORT_MANAGER_H

#include <string>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/application/timeline/json_process_enum.h"
#include "analysis/csrc/application/include/export_mode_enum.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
class ExportManager {
public:
    explicit ExportManager(const std::string &profPath) : profPath_(profPath) {}
    ExportManager(const std::string& profPath, const std::string& jsonPath) : profPath_(profPath),
        jsonPath_(jsonPath)
    {}
    bool Run(ExportMode exportMode);
private:
    bool Init();
    bool CheckProfDirsValid();
    bool ProcessData(DataInventory &dataInventory, ExportMode exportMode);
    std::vector<JsonProcess> GetProcessEnum();

private:
    std::string profPath_;
    std::string jsonPath_;
};
}
}

#endif // ANALYSIS_APPLICATION_EXPORT_MANAGER_H
