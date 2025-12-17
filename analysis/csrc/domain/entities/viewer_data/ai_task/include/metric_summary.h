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
