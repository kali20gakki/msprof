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
    using ORI_DATA_FORMAT = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double,
                                                   double, std::string, std::string, int32_t>>;
    // start, end, name, deviceId, connectionId, correlationId, globalPid, taskType, contextId, streamId, taskId,
    // modelId
    using PROCESSED_DATA_FORMAT = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, int32_t, uint64_t,
                                                         uint32_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>>;
public:
    TaskProcesser() = default;
    TaskProcesser(const std::string &reportDBPath, const std::vector<std::string> &profPaths);
    virtual ~TaskProcesser() = default;
protected:
    void Process(const std::string &fileDir) override;
private:
    ORI_DATA_FORMAT GetData(const std::string &dbPath);
    PROCESSED_DATA_FORMAT FormatData(const ORI_DATA_FORMAT &oriData);
    bool SaveData(const PROCESSED_DATA_FORMAT &processedData);
    DBInfo ascendTaskDB_;
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TASK_PROCESSER_H
