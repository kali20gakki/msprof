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

#include "analysis/csrc/utils/time_utils.h"
#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成TASK表
class TaskProcesser : public TableProcesser {
    // model_id, index_id, stream_id, task_id, context_id, batch_id, start_time, duration, host_task_type,
    // device_task_type, connection_id
    using OriDataFormat = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double,
                                                   double, std::string, std::string, uint32_t>>;
    // start, end, deviceId, connectionId, correlationId, globalPid, taskType, contextId, streamId, taskId,
    // modelId
    using ProcessedDataFormat = std::vector<std::tuple<std::string, std::string, uint32_t, int64_t, uint64_t,
                                                         uint64_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>>;
public:
    TaskProcesser() = default;
    TaskProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    virtual ~TaskProcesser() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(DBInfo &ascendTaskDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData, uint16_t deviceId, uint32_t globalPid,
                                   uint16_t platformVersion, Utils::ProfTimeRecord &timeRecord);
    static uint64_t GetTaskType(const std::string &hostType, const std::string &deviceType, uint16_t platformVersion);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TASK_PROCESSER_H
