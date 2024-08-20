/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compute_task_info_processor.h
 * Description        : compute_task_info processor，处理taskInfo表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/1
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_COMPUTE_TASK_INFO_PROCESSOR_H
#define ANALYSIS_DOMAIN_COMPUTE_TASK_INFO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/task_info_data.h"

namespace Analysis {
namespace Domain {
// hashid, model_id, op_name, stream_id, task_id, block_dim, mix_block_dim, task_type, op_type,
// op_flag, batch_id, input_formats, input_data_types, input_shapes, output_formats,
// output_data_types, output_shapes, device_id, context_id
using TaskInfoFormat = std::tuple<std::string, uint32_t, std::string, uint32_t, uint32_t, uint32_t, uint32_t,
                                  std::string, std::string, std::string, uint32_t, std::string, std::string,
                                  std::string, std::string, std::string, std::string, uint16_t, uint32_t>;
class ComputeTaskInfoProcessor : public DataProcessor {
public:
    ComputeTaskInfoProcessor() = default;
    explicit ComputeTaskInfoProcessor(const std::string &profPath);

private:
    bool Process(DataInventory &dataInventory) override;
    std::vector<TaskInfoData> LoadData(const DBInfo &taskInfoDB, const std::string &dbPath,
                                         std::unordered_map<std::string, std::string>& hashMap);
};
}
}

#endif // ANALYSIS_DOMAIN_COMPUTE_TASK_INFO_PROCESSOR_H
