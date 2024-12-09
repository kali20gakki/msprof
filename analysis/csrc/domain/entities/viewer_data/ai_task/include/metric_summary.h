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

#ifndef ANALYSIS_DOMAIN_METRIC_SUMMARY_H
#define ANALYSIS_DOMAIN_METRIC_SUMMARY_H

#include <map>
#include <string>
#include <utility>
#include <vector>
#include "analysis/csrc/domain/valueobject/include/task_id.h"

namespace Analysis {
namespace Domain {
struct MetricSummary {
    std::vector<std::string> labels;
    std::map<TaskId, std::vector<std::vector<std::string>>> data;
    MetricSummary() = default;
    MetricSummary(std::vector<std::string> _labels,
                  std::map<TaskId, std::vector<std::vector<std::string>>> _data)
        : labels(std::move(_labels)), data(std::move(_data)) {}
};
}
}
#endif // ANALYSIS_DOMAIN_METRIC_SUMMARY_H
