/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ddr_data.h
 * Description        : ddr_processor处理ddr表后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/8/10
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_DDR_DATA_H
#define ANALYSIS_DOMAIN_DDR_DATA_H

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct DDRData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    double fluxRead = 0.0;
    double fluxWrite = 0.0;
};
}
}
#endif // ANALYSIS_DOMAIN_DDR_DATA_H
