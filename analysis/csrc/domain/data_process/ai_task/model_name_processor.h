/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : model_name_processor.h
 * Description        :处理model_name表数据
 * Author             : msprof team
 * Creation Date      : 2025/5/12
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_MODEL_NAME_PROCESSOR_H
#define ANALYSIS_DOMAIN_MODEL_NAME_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/model_name_data.h"

namespace Analysis {
namespace Domain {
// model_id, model_name
using OriModelNameDataFormat = std::vector<std::tuple<uint64_t, std::string>>;
class ModelNameProcessor : public DataProcessor {
public:
    ModelNameProcessor() = default;
    explicit ModelNameProcessor(const std::string &profPath);

private:
    bool Process(DataInventory& dataInventory) override;
    OriModelNameDataFormat LoadData(const DBInfo &modelNameDB, const std::string &dbPath);
    std::vector<ModelName> FormatData(const OriModelNameDataFormat &oriData);
};
}
}
#endif // ANALYSIS_DOMAIN_MODEL_NAME_PROCESSOR_H