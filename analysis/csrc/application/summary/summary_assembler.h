/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : summary_assembler.h
 * Description        : summary类型导出头文件
 * Author             : msprof team
 * Creation Date      : 2024/12/4
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SUMMARY_ASSEMBLER_H
#define ANALYSIS_APPLICATION_SUMMARY_ASSEMBLER_H

#include <cstdint>
#include <map>
#include "analysis/csrc/infrastructure/data_inventory/include/data_inventory.h"
#include "analysis/csrc/application/summary/summary_constant.h"

namespace Analysis {
namespace Application {
using namespace Analysis::Infra;
class SummaryAssembler {
public:
    SummaryAssembler() = default;
    SummaryAssembler(const std::string &name, const std::string &profPath)
        : processorName_(name), profPath_(profPath) {}
    bool Run(DataInventory &dataInventory)
    {
        INFO("Begin to Assemble % data", processorName_);
        auto res = AssembleData(dataInventory);
        if (res == ASSEMBLE_SUCCESS) {
            INFO("Assemble and export % data success", processorName_);
            return true;
        } else if (res == DATA_NOT_EXIST) {
            WARN("No need to Export % data", processorName_);
            return true;
        } else {
            ERROR("Export % data failed");
            return false;
        }
    }
protected:
    std::string processorName_;
    std::string profPath_;
    std::vector<std::vector<std::string>> res_;
    std::vector<std::string> headers_;
private:
    virtual uint8_t AssembleData(DataInventory &dataInventory) = 0;
    virtual void WriteToFile() {}
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_ASSEMBLER_H
