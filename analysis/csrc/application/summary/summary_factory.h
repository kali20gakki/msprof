/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : summary_factory.h
 * Description        : summary_factory，summary工厂类
 * Author             : msprof team
 * Creation Date      : 2025/5/22
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_SUMMARY_FACTORY_H
#define ANALYSIS_APPLICATION_SUMMARY_FACTORY_H

#include <functional>
#include "analysis/csrc/application/summary/summary_assembler.h"

namespace Analysis {
namespace Application {
using AssemblerCreator = std::function<void(const std::string& profPath, std::shared_ptr<SummaryAssembler>& assembler)>;

class SummaryFactory {
public:
    static std::shared_ptr<SummaryAssembler> GetAssemblerByName(const std::string& processName,
                                                                const std::string& profPath);
private:
    static std::unordered_map<std::string, AssemblerCreator> assemblerTable_;
};
}
}
#endif // ANALYSIS_APPLICATION_SUMMARY_FACTORY_H
