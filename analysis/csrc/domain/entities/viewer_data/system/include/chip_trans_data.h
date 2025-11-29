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
