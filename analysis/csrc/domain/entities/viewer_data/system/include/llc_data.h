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

#ifndef ANALYSIS_DOMAIN_LLC_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_LLC_FORMAT_DATA_H

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct LLcData : public BasicData {
    uint8_t llcID = UINT8_MAX; // 原始数据对应的格式是I，对应uint32,但是实际来看uint8完全够了，节省内存
    uint16_t deviceId = UINT16_MAX;
    double hitRate = 0.0; // %,中间db里面是小数，在这里统一变成百分数，比如50就是代表50%
    double throughput = 0; // MB/s
    std::string mode; // read和write,从json文件里面读
};

struct LLcSummaryData {
    uint8_t llcId = UINT8_MAX; // llcId为UINT8_MAX，表示Average,否则表示任务ID
    uint16_t deviceId = UINT16_MAX;
    double hitRate = 0;
    double throughput = 0;
    std::string mode;
};
}
}

#endif // ANALYSIS_DOMAIN_LLC_FORMAT_DATA_H
