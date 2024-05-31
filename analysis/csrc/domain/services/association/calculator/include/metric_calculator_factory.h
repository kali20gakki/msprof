/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : metric_calculator_factory.h
 * Description        : metric计算工厂类
 * Author             : msprof team
 * Creation Date      : 2024/5/21
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_METRIC_CALCULATOR_FACTORY_H
#define ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_METRIC_CALCULATOR_FACTORY_H

#include <functional>
#include "analysis/csrc/domain/services/association/calculator/metric/metric_calculator.h"

namespace Analysis {
namespace Domain {
using Creator = std::function<std::unique_ptr<MetricCalculator>()>;

class MetricCalculatorFactory {
public:
    static std::unique_ptr<MetricCalculator> GetAicCalculator(AicMetricsEventsType type)
    {
        auto it = aicEvent.find(type);
        if (it != aicEvent.end()) {
            return it->second();
        }
        return nullptr;
    }

    static std::unique_ptr<MetricCalculator> GetAivCalculator(AivMetricsEventsType type)
    {
        auto it = aivEvent.find(type);
        if (it != aivEvent.end()) {
            return it->second();
        }
        return nullptr;
    }
private:
    static std::unordered_map<AicMetricsEventsType, Creator> aicEvent;
    static std::unordered_map<AivMetricsEventsType, Creator> aivEvent;
};
}
}
#endif // ANALYSIS_DOMAIN_SERVICES_ASSOCIATION_METRIC_CALCULATOR_FACTORY_H
