/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : fusion_op_processor.h
 * Description        : fusion_op，处理ge_fusion_op_info表数据
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_FUSION_OP_PROCESSOR_H
#define ANALYSIS_DOMAIN_FUSION_OP_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/fusion_op_data.h"

namespace Analysis {
namespace Domain {
// model_id, fusion_name, fusion_op_nums, op_names, memory_input,
// memory_output, memory_weight, memory_workspace, memory_total
using OriFusionOpDataFormat = std::vector<std::tuple<uint64_t, std::string, uint64_t, std::string,
        std::string, std::string, std::string, std::string, std::string>>;

class FusionOpProcessor : public DataProcessor {
public:
    FusionOpProcessor() = default;
    explicit FusionOpProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    OriFusionOpDataFormat LoadData(const DBInfo &fusionOpDB, const std::string &dbPath);
    std::vector<FusionOpInfo> FormatData(const OriFusionOpDataFormat &oriData);
};
}
}
#endif // ANALYSIS_DOMAIN_FUSION_OP_PROCESSOR_H