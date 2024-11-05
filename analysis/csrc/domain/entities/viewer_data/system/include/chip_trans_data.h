/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip_trans_data.h
 * Description        : PaLinkInfo,PcieInfo表相关数据数据格式化后的数据结构
 * Author             : msprof team
 * Creation Date      : 2024/08/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_CHIP_TRANS_DATA_H
#define ANALYSIS_DOMAIN_CHIP_TRANS_DATA_H

#include <cstdint>
#include <string>

namespace Analysis {
namespace Domain {
struct PaLinkInfoData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t pa_link_id;
    std::string pa_link_traffic_monit_rx;
    std::string pa_link_traffic_monit_tx;
    uint64_t local_time;
};
struct PcieInfoData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t pcie_id;
    uint64_t pcie_write_bandwidth;
    uint64_t pcie_read_bandwidth;
    uint64_t local_time;
};

}
}

#endif // ANALYSIS_DOMAIN_CHIP_TRANS_DATA_H
