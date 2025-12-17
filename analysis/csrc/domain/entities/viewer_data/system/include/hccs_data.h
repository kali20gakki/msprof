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

#ifndef ANALYSIS_DOMAIN_HCCS_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_HCCS_FORMAT_DATA_H

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct HccsData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    double txThroughput = 0; // MB/s
    double rxThroughput = 0; // MB/s
};

struct HccsSummaryData {
    uint16_t deviceId = UINT16_MAX;
    double txMaxThroughput = 0; // 最大带宽（MB/s）
    double txMinThroughput = 0; // 最小带宽（MB/s）
    double txAvgThroughput = 0; // 平均带宽（MB/s）
    double rxMaxThroughput = 0;
    double rxMinThroughput = 0;
    double rxAvgThroughput = 0;
};
}
}

#endif // ANALYSIS_DOMAIN_HCCS_FORMAT_DATA_H
