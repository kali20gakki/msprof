/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : task_processor.h
 * Description        : 处理ascendTask表数据
 * Author             : msprof team
 * Creation Date      : 2024/8/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_ASCEND_TASK_PROCESSOR_H
#define ANALYSIS_DOMAIN_ASCEND_TASK_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/ascend_task_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/memcpy_info_data.h"

namespace Analysis {
namespace Domain {
struct MemcpyRecord {
    uint64_t dataSize;  // memcpy的数据量
    uint64_t maxSize;  // 每个task能拷贝的最大数据量
    uint64_t remainSize;  // 还剩下多少数据量需要后面的task去拷贝
    uint16_t operation;  // 拷贝类型
};
using MEMCPY_ASYNC_FORMAT = std::unordered_map<uint64_t, MemcpyRecord>;

// start_time, duration, model_id, index_id, stream_id, task_id, context_id, batch_id, connection_id host_task_type,
// device_task_type
using OriAscendTaskData = std::vector<std::tuple<double, double, uint32_t, int32_t, uint32_t, uint32_t, uint32_t,
                                                 uint32_t, uint64_t, std::string, std::string>>;
class TaskProcessor : public DataProcessor {
public:
    TaskProcessor() = default;
    explicit TaskProcessor(const std::string &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    bool ProcessSingleDevice(const std::string &devicePath, std::vector<AscendTaskData> &allProcessedData);
    OriAscendTaskData LoadData(const DBInfo &ascendTaskDB, const std::string &dbPath);
    std::vector<AscendTaskData> FormatData(const OriAscendTaskData &oriData,
                                           const Utils::ProfTimeRecord &timeRecord,
                                           const uint16_t deviceId);

    std::string GetTaskType(const std::string &hostType, const std::string &deviceType, uint16_t platformVersion);
    bool GenerateMemcpyRecordMap();
private:
    MEMCPY_ASYNC_FORMAT memcpyRecordMap_;  // 存储connectionId与memcpy_info数据的映射
    std::vector<MemcpyInfoData> processedMemcpyInfo_;
};
}
}

#endif // ANALYSIS_DOMAIN_ASCEND_TASK_PROCESSOR_H
