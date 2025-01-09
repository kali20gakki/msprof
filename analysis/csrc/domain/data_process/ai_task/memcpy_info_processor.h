/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : memcpy_info_processor.h
 * Description        : memcpy_info_processor，处理MemcpyInfo表数据
 * Author             : msprof team
 * Creation Date      : 2025/01/07
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MEMCPY_INFO_PROCESSOR_H
#define ANALYSIS_DOMAIN_MEMCPY_INFO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"

namespace Analysis {
namespace Domain {
using MemcpyInfoFormat = std::vector<std::tuple<uint32_t, uint16_t, uint16_t, uint32_t, uint16_t, int64_t, uint16_t>>;
class MemcpyInfoProcessor : public DataProcessor {
public:
    MemcpyInfoProcessor() = default;
    explicit MemcpyInfoProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    MemcpyInfoFormat LoadData(const DBInfo &runtimeDB, const std::string &dbPath);
    std::vector<MemcpyInfoData> FormatData(const MemcpyInfoFormat &oriData);
};
}
}
#endif // ANALYSIS_DOMAIN_MEMCPY_INFO_PROCESSOR_H