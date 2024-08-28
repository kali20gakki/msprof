/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chip_trans_processor.h
 * Description        : chip_trans_processor，处理PaLinkInfo,PcieInfo表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/08/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_CHIP_TRANS_PROCESSOR_H
#define ANALYSIS_DOMAIN_CHIP_TRANS_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/chip_trans_data.h"


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
                    ChipTransData& chipTransData);
};
}
}


#endif // ANALYSIS_DOMAIN_CHIP_TRANS_PROCESSOR_H
