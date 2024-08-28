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

#include <cstdint>

namespace Analysis {
namespace Domain {
struct BandwidthData {
    uint32_t min = UINT32_MAX;
    uint32_t max = UINT32_MAX;
    uint32_t avg = UINT32_MAX;
};

struct PCIeData {
    uint16_t deviceId = UINT16_MAX;
    uint64_t timestamp = UINT64_MAX;
    BandwidthData txPost;
    BandwidthData txNopost;
    BandwidthData txCpl;
    BandwidthData txNopostLatency;
    BandwidthData rxPost;
    BandwidthData rxNopost;
    BandwidthData rxCpl;
};
}
}
#endif // ANALYSIS_DOMAIN_PCIE_DATA_H
