/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : data_processor_factory.h
 * Description        : data_processor_factory，processor工厂类
 * Author             : msprof team
 * Creation Date      : 2024/9/3
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_DATA_PROCESSOR_FACTORY_H
#define ANALYSIS_DOMAIN_DATA_PROCESSOR_FACTORY_H

#include <functional>
#include "analysis/csrc/domain/data_process/data_processor.h"

namespace Analysis {
namespace Domain {
using ProcessorCreator = std::function<void(const std::string&, std::shared_ptr<DataProcessor>&)>;
class DataProcessorFactory {
public:
    static std::shared_ptr<DataProcessor> GetDataProcessByName(const std::string &profPath,
                                                               const std::string &processName);
private:
    static std::unordered_map<std::string, ProcessorCreator> processorTable_;
};
}
}
#endif // ANALYSIS_DOMAIN_DATA_PROCESSOR_FACTORY_H
