/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : qos_processor.h
 * Description        : qos_processor，处理qos表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/8/28
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_QOS_PROCESSOR_H
#define ANALYSIS_DOMAIN_QOS_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/system/include/qos_data.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

// timestamp, bw1, ..., bw10
using OriQosData = std::vector<std::tuple<double, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t>>;
// 该类用于定义处理qos.db中QosBwData表
class QosProcessor : public DataProcessor {
public:
    QosProcessor() = default;
    explicit QosProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<QosData> &allProcessedData);
    OriQosData LoadData(const DBInfo &qosDB);
    std::vector<QosData> FormatData(const OriQosData &oriData, const LocaltimeContext &localtimeContext);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_QOS_PROCESSOR_H
