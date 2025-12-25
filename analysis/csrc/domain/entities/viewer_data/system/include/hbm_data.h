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

#ifndef ANALYSIS_DOMAIN_HBM_FORMAT_DATA_H
#define ANALYSIS_DOMAIN_HBM_FORMAT_DATA_H

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct HbmData : public BasicData {
    uint8_t hbmId = UINT8_MAX; // 目前只有0, 1, 2, 3
    uint16_t deviceId = UINT16_MAX;
    double bandWidth = 0; // MB/s
    std::string eventType; // read和write
};

struct HbmSummaryData {
    uint16_t deviceId = UINT16_MAX;
    uint8_t hbmId = UINT8_MAX; // hbmId为UINT8_MAX，表示Average,否则表示内存访问单元的ID
    double readBandwidth = 0; // 读带宽（MB/s）
    double writeBandwidth = 0; // 写带宽（MB/s）
};
}
}

#endif // ANALYSIS_DOMAIN_HBM_FORMAT_DATA_H
