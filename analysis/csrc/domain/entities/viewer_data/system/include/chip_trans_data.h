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

#include <string>
#include "analysis/csrc/domain/entities/viewer_data/basic_data.h"

namespace Analysis {
namespace Domain {
struct PaLinkInfoData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t paLinkId;
    std::string paLinkTrafficMonitRx;
    std::string paLinkTrafficMonitTx;
};
struct PcieInfoData : public BasicData {
    uint16_t deviceId = UINT16_MAX;
    uint32_t pcieId;
    uint64_t pcieWriteBandwidth;
    uint64_t pcieReadBandwidth;
};

}
}

#endif // ANALYSIS_DOMAIN_CHIP_TRANS_DATA_H
