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

#ifndef ANALYSIS_DOMAIN_CHIP_TRANS_PROCESSOR_H
#define ANALYSIS_DOMAIN_CHIP_TRANS_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"


namespace Analysis {
namespace Domain {
// pa_link_id, pa_link_traffic_monit_rx, pa_link_traffic_monit_tx, sys_time
using OriPaFormat = std::vector<std::tuple<uint32_t, std::string, std::string, uint64_t>>;
// pcie_id, pcie_write_bandwidth, pcie_read_bandwidth, sys_time
using OriPcieFormat = std::vector<std::tuple<uint32_t, uint64_t, uint64_t, uint64_t>>;

class ChipTransProcessor : public DataProcessor {
public:
    struct ChipTransData {
        std::vector<PaLinkInfoData> resPaData;
        std::vector<PcieInfoData> resPcieData;
        OriPaFormat oriPaData;
        OriPcieFormat oriPcieData;
        Utils::SyscntConversionParams params;
        Utils::ProfTimeRecord timeRecord;
    };
    ChipTransProcessor() = default;
    explicit ChipTransProcessor(const std::string& profPaths);
    virtual ~ChipTransProcessor() = default;
private:
    bool Process(DataInventory& dataInventory) override;
    OriPaFormat LoadPaData(const DBInfo& paLinkInfo);
    OriPcieFormat LoadPcieData(const DBInfo& pcieInfo);
    bool ProcessOneDevice(const std::string& devicePath, ChipTransData& chipTransData);
    bool FormatData(std::vector<PaLinkInfoData>& paFormatData, std::vector<PcieInfoData>& pcieFormatData,
                    ChipTransData& chipTransData, const uint16_t &deviceId);
};
}
}


#endif // ANALYSIS_DOMAIN_CHIP_TRANS_PROCESSOR_H
