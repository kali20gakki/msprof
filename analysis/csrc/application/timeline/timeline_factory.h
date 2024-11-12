/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : timeline_factory.h
 * Description        : timeline_factory，processor工厂类
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_APPLICATION_TIMELINE_FACTORY_H
#define ANALYSIS_APPLICATION_TIMELINE_FACTORY_H

#include <functional>
#include "analysis/csrc/application/timeline/json_assembler.h"

namespace Analysis {
namespace Application {
using AssemblerCreator = std::function<void(std::shared_ptr<JsonAssembler> &assembler)>;
class TimelineFactory {
public:
    static std::shared_ptr<JsonAssembler> GetAssemblerByName(const std::string &processName);
private:
    static std::unordered_map<std::string, AssemblerCreator> assemblerTable_;
};
}
}
#endif // ANALYSIS_APPLICATION_TIMELINE_FACTORY_H
