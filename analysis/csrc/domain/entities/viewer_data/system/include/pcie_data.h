/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : pcie_data.h
 * Description        : pcie_processor处理PcieOriginalData后的格式化数据
 * Author             : msprof team
 * Creation Date      : 2024/08/13
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_PCIE_DATA_H
#define ANALYSIS_DOMAIN_PCIE_DATA_H

#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct BandwidthData {
    // 无数据时默认值设置为0
    uint32_t min = 0;
    uint32_t max = 0;
    uint32_t avg = 0;
};

struct PCIeData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    BandwidthData txPost;
    BandwidthData txNonpost;
    BandwidthData txCpl;
    BandwidthData txNonpostLatency;
    BandwidthData rxPost;
    BandwidthData rxNonpost;
    BandwidthData rxCpl;
};
}
}
#endif // ANALYSIS_DOMAIN_PCIE_DATA_H
