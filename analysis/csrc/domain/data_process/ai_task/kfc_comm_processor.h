/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : kfc_comm_processor.cpp
 * Description        : kfc通信数据导出流程
 * Author             : msprof team
 * Creation Date      : 2024/8/30
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_KFC_COMM_PROCESSOR_H
#define ANALYSIS_DOMAIN_KFC_COMM_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/kfc_turn_data.h"

namespace Analysis {
namespace Domain {

// "model_id", "index_id", "op_name", "op_timestamp", "op_duration", "iteration", "hccl_name",
// "group_name", "plane_id", "timestamp", "duration", "stream_id", "task_id", "duration_estimated", "local_rank",
// "remote_rank", "transport_type",
// "size", "data_type", "link_type", "bandwidth", "context_id", "notify_id", "batch_id", "rdma_type",
using OriKfcTaskData = std::vector<std::tuple<uint32_t, uint32_t, std::string, uint64_t, uint64_t, uint32_t,
        std::string, std::string, int32_t, uint64_t, uint64_t, uint32_t, uint32_t, uint64_t, uint32_t, uint32_t,
        std::string, uint32_t, std::string, std::string, double, uint32_t, std::string, uint32_t, std::string>>;
// "model_id", "index_id", "op_name", "timestamp", "duration", "group_name", "connection_id", "data_type", "alg_type",
// "count", "rank_size
using OriKfcOPData = std::vector<std::tuple<uint32_t, uint32_t, std::string, uint64_t, uint64_t, std::string,
uint64_t, std::string, std::string, uint64_t, uint64_t>>;

struct KfcTaskDto {
    uint32_t modelId;
    uint32_t indexId;
    std::string opName;
    uint64_t opTimestamp;
    uint64_t opDuration;
    uint32_t iteration;
    std::string hcclName;
    std::string groupName;
    int32_t planeId;
    uint64_t timestamp;
    uint64_t duration;
    uint32_t streamId;
    uint32_t taskId;
    uint64_t durationEstimated;
    uint32_t localRank;
    uint32_t remoteRank;
    std::string transportType;
    uint32_t size;
    std::string dataType;
    std::string linkType;
    double bandwidth;
    uint32_t contextId;
    std::string notifyId;
    uint32_t batchId;
    std::string rdmaType;
};

struct KfcOPDto {
    uint32_t modelId;
    uint32_t indexId;
    std::string opName;
    uint64_t timestamp;
    uint64_t duration;
    std::string groupName;
    uint64_t connectionId;
    std::string dataType;
    std::string algType;
    uint64_t count;
    uint64_t rankSize;
};

class KfcCommProcessor : public DataProcessor {
public:
    KfcCommProcessor() = default;
    explicit KfcCommProcessor(const std::string &profPath);
private:
    bool Process(DataInventory& dataInventory) override;
    std::vector<KfcTaskDto> LoadTaskData(const DBInfo &kfcDB, const std::string &dbPath);
    std::vector<KfcOPDto> LoadOPData(const DBInfo &kfcDB, const std::string &dbPath);
    std::vector<KfcTaskData> FormatTaskData(const std::vector<KfcTaskDto> &oriCommData,
                                            std::unordered_map<std::string, std::string>& hashMap, uint16_t deviceId);
    std::vector<KfcOpData> FormatOPData(const std::vector<KfcOPDto> &oriComputeData,
                                        std::unordered_map<std::string, std::string>& hashMap, uint16_t deviceId);
};
}
}


#endif // ANALYSIS_DOMAIN_KFC_COMM_PROCESSOR_H
