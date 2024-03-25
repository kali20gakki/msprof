/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : compute_task_info_processor.h
 * Description        : compute_task_info_processor，处理TaskInfo表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_COMPUTE_TASK_INFO_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_COMPUTE_TASK_INFO_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成COMPUTE_TASK_INFO表
class ComputeTaskInfoProcessor : public TableProcessor {
    // model_id, op_name, stream_id, task_id, block_dim, mix_block_dim, task_type, op_type,
    // timestamp, batch_id, input_formats, input_data_types, input_shapes, output_formats,
    // output_data_types, output_shapes, device_id, context_id
    using OriDataFormat = std::vector<std::tuple<uint32_t, std::string, int32_t, int32_t, uint32_t, uint32_t,
                                                 std::string, std::string, double, uint32_t, std::string,
                                                 std::string, std::string, std::string, std::string, std::string,
                                                 int32_t, uint32_t>>;
    // name, globalTaskId, block_dim, mixBlockDim, taskType, opType, inputFormats, inputDataTypes, inputShapes,
    // outputFormats, outputDataTypes, outputShapes
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, uint32_t, uint64_t, uint64_t,
                                                       uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t>>;
public:
    ComputeTaskInfoProcessor() = default;
    ComputeTaskInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~ComputeTaskInfoProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(const DBInfo &geInfo);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_COMPUTE_TASK_INFO_PROCESSOR_H
