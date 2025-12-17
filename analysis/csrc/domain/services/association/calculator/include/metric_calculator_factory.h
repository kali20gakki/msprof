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
