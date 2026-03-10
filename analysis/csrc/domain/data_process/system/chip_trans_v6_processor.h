/* -------------------------------------------------------------------------
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
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

#ifndef ANALYSIS_DOMAIN_CHIP_TRANS_V6_PROCESSOR_H
#define ANALYSIS_DOMAIN_CHIP_TRANS_V6_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/chip_trans_data.h"


namespace Analysis {
namespace Domain {
// die_id, pcie_write_bandwidth, pcie_read_bandwidth, sys_time
using OriPcieV6Format = std::vector<std::tuple<uint32_t, uint64_t, uint64_t, uint64_t>>;

class ChipTransV6Processor : public DataProcessor {
public:
    struct ChipTransV6Data {
        std::vector<PcieInfoV6Data> resPcieV6Data;
        OriPcieV6Format oriPcieV6Data;
        Utils::SyscntConversionParams params;
        Utils::ProfTimeRecord timeRecord;
    };
    ChipTransV6Processor() = default;
    explicit ChipTransV6Processor(const std::string& profPaths);
    virtual ~ChipTransV6Processor() = default;
private:
    friend class ChipTransProcessor;
    bool Process(DataInventory& dataInventory) override;
    OriPcieV6Format LoadPcieV6Data(const DBInfo& pcieInfo);
    bool ProcessOneDevice(const std::string& devicePath, ChipTransV6Data& chipTransV6Data);
    bool FormatData(std::vector<PcieInfoV6Data>& pcieV6FormatData,
                    ChipTransV6Data& chipTransV6Data, const uint16_t &deviceId);
};
}
}


#endif // ANALYSIS_DOMAIN_CHIP_TRANS_V6_PROCESSOR_H