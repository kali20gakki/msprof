/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : sio_processor.h
 * Description        : sio_processor，处理sio表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/28
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_SIO_PROCESSOR_H
#define ANALYSIS_DOMAIN_SIO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/sio_data.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

// die_id, timestamp, req_rx, rsp_rx, snp_rx, dat_rx, req_tx, rsp_tx, snp_tx, dat_tx
using OriSioData = std::vector<std::tuple<uint32_t, double, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t>>;
// 该类用于定义处理sio.db中Sio表
class SioProcessor : public DataProcessor {
public:
    SioProcessor() = default;
    explicit SioProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<SioData> &allProcessedData);
    OriSioData LoadData(const DBInfo &sioDB);
    std::vector<SioData> FormatData(const OriSioData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_SIO_PROCESSOR_H
