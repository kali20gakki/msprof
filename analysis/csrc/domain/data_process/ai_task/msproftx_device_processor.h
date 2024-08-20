/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : msprofTx_device_processor.h
 * Description        : 处理device侧stepTrace表中的msprofTx数据
 * Author             : msprof team
 * Creation Date      : 2024/8/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MSPROFTX_PROCESSOR_H
#define ANALYSIS_DOMAIN_MSPROFTX_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"

namespace Analysis {
namespace Domain {
// model_id, index_id, stream_id, task_id, timestamp
using OriMsprofTxDeviceData = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, double>>;

class MsprofTxDeviceProcessor : public DataProcessor {
public:
    MsprofTxDeviceProcessor() = default;
    explicit MsprofTxDeviceProcessor(const std::string  &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    OriMsprofTxDeviceData LoadData(const DBInfo &stepTraceDB, const std::string &dbPath);
    bool ProcessOneDevice(const Utils::ProfTimeRecord &record, std::vector<MsprofTxDeviceData> &res,
                          const std::string &devPath);
    std::vector<MsprofTxDeviceData> FormatData(const OriMsprofTxDeviceData &oriData,
                                               const Utils::ProfTimeRecord &record,
                                               const uint16_t deviceId,
                                               const Utils::SyscntConversionParams &params);
};
}
}

#endif // ANALYSIS_DOMAIN_MSPROFTX_PROCESSOR_H
