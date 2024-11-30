/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_processor.h
 * Description        : task_processor，处理AscendTask表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_TASK_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_TASK_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成TASK表
class TaskProcessor : public TableProcessor {
    // model_id, index_id, stream_id, task_id, context_id, batch_id, start_time, duration, host_task_type,
    // device_task_type, connection_id
    using OriDataFormat = std::vector<std::tuple<uint32_t, int32_t, int32_t, uint32_t, uint32_t, uint32_t, double,
                                                   double, std::string, std::string, uint32_t>>;
    // start, end, deviceId, connectionId, globalTaskId, globalPid, taskType, contextId, streamId, taskId,
    // modelId
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint32_t, int64_t, uint64_t,
                                                       uint64_t, uint32_t, uint32_t, int32_t, uint32_t, uint32_t>>;
    // model_id, index_id, stream_id, task_id, timestamp
    using OriMsprofTxDataFormat = std::vector<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t, double>>;
public:
    TaskProcessor() = default;
    TaskProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~TaskProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    bool ProcessTaskData(const std::string &fileDir, const std::vector<std::string> &deviceList,
                         ThreadData threadData, uint32_t pid);
    static OriDataFormat GetData(DBInfo &ascendTaskDB);
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData, const ThreadData &threadData,
                                          uint16_t platformVersion, uint32_t pid);
    static uint64_t GetTaskType(const std::string &hostType, const std::string &deviceType, uint16_t platformVersion);
    bool ProcessMsprofTxData(const std::string &fileDir, const std::vector<std::string> &deviceList,
                             ThreadData threadData, uint32_t pid);
    static OriMsprofTxDataFormat GetMsprofTxTaskData(DBInfo &stepTraceDB);
    static ProcessedDataFormat FormatMsprofTxTaskData(const OriMsprofTxDataFormat &oriData,
                                                      const ThreadData &threadData,
                                                      Utils::SyscntConversionParams &params,
                                                      uint32_t pid);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_TASK_PROCESSOR_H
