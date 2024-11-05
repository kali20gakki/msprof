/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_processor.h
 * Description        : api_processor，处理api_event表数据
 * Author             : msprof team
 * Creation Date      : 2024/7/31
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_API_PROCESSOR_H
#define ANALYSIS_DOMAIN_API_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/api_data.h"

namespace Analysis {
namespace Domain {
// struct_type, id, level, thread_id, item_id, start, end, connection_id
using OriApiDataFormat = std::vector<std::tuple<std::string, std::string, std::string, uint32_t,
        std::string, uint64_t, uint64_t, uint64_t>>;
class ApiProcessor : public DataProcessor {
public:
    ApiProcessor() = default;
    explicit ApiProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    OriApiDataFormat LoadData(const DBInfo &apiDB, const std::string &dbPath);
    std::vector<ApiData> FormatData(const OriApiDataFormat &oriData, const Utils::ProfTimeRecord &record,
                                          const Utils::SyscntConversionParams &params);
};
}
}
#endif // ANALYSIS_DOMAIN_API_PROCESSOR_H
