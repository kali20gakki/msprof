/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hbm_data.h
 * Description        : hbm_processor处理hbm表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/20
 * *****************************************************************************
 */

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
