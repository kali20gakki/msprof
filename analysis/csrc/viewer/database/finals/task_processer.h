/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_proceser.h
 * Description        : task_processer，处理AscendTask表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_TASK_PROCESSER_H
#define ANALYSIS_VIEWER_DATABASE_TASK_PROCESSER_H

#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成TASK表
class TaskProcesser : public TableProcesser {
    // model_id, index_id, stream_id, task_id, context_id, batch_id, start_time, duration, host_task_type,
    // device_task_type, connection_id
    using OriDataFormat = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double,
                                                   double, std::string, std::string, int64_t>>;
    // start, end, deviceId, connectionId, correlationId, globalPid, taskType, contextId, streamId, taskId,
    // modelId
    using ProcessedDataFormat = std::vector<std::tuple<double, double, uint32_t, int64_t, uint64_t,
                                                         uint64_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>>;
public:
    TaskProcesser() = default;
    TaskProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    virtual ~TaskProcesser() = default;
protected:
    bool Process(const std::string &fileDir) override;
private:
    OriDataFormat GetData();
    ProcessedDataFormat FormatData(const OriDataFormat &oriData);
    uint64_t GetTaskType(const std::string &hostType, const std::string &deviceType);
    uint64_t GetConnectionId(uint64_t high, uint64_t low);
    DBInfo ascendTaskDB_;
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TASK_PROCESSER_H
