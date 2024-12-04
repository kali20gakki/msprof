/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : export_summary.h
 * Description        : 导出op_summary
 * Author             : msprof team
 * Creation Date      : 2024/12/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_EXPORT_SUMMARY_H
#define ANALYSIS_APPLICATION_EXPORT_SUMMARY_H

#include <string>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
class ExportSummary {
public:
    explicit ExportSummary(const std::string &profPath) : profPath_(profPath) {}
    bool Run();
private:
    bool Init();
    bool CheckProfDirsValid();
    bool ProcessData(DataInventory &dataInventory);
private:
    std::string profPath_;
};
}
}

#endif // ANALYSIS_APPLICATION_EXPORT_SUMMARY_H
