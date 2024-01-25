/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication__info_processor.h
 * Description        : communication_info_processor，处理HCCLSingleDevice表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_COMMUNICATION_INFO_PROCESSOR_H
#define ANALYSIS_VIEWER_DATABASE_COMMUNICATION_INFO_PROCESSOR_H

#include "analysis/csrc/viewer/database/finals/table_processor.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "analysis/csrc/utils/time_utils.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于依据HCCLSingelDevice表生成COMMUNICATION_TASK_INFO(通信小算子)和COMMUNICATION_OP表(通信大算子)
class CommunicationInfoProcessor : public TableProcessor {
    // model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, remote_rank,
    // transport_type, size, data_type, link_type, context_id, notify_id, batch_id, rdma_type, timestamp, duration,
    // connection_id
    using HcclFormat = std::tuple<uint32_t, std::string, std::string, std::string, int32_t, uint64_t, uint32_t,
                                  uint32_t, uint32_t, std::string, uint64_t, std::string, std::string, uint32_t,
                                  uint64_t, uint32_t, std::string, double, double, uint32_t>;
    using OriDataFormat = std::vector<HcclFormat>;
    // name, correlationId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
                                                               uint64_t, uint64_t, uint32_t, uint32_t, uint64_t,
                                                               uint64_t, uint64_t, uint64_t, uint32_t>>;
    // opName, start, end, connectionId, group_name, opId
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, std::string, std::string, uint64_t, uint64_t,
                                                             uint64_t>>;
    // 不同线程需要实例化的数据集合，由于函数行数限制定义为结构体
    struct ThreadData {
        uint16_t deviceId = UINT16_MAX;
        uint32_t globalPid = UINT32_MAX;
        Utils::ProfTimeRecord timeRecord;
        DBInfo hcclSingleDeviceDB{"hccl_single_device.db", "HCCLSingleDevice"};
    };
    struct CommunicationTaskData {
        int32_t planeId = INT32_MAX;
        uint32_t modelId = UINT32_MAX;
        uint32_t streamId = UINT32_MAX;
        uint32_t taskId = UINT32_MAX;
        uint32_t contextId = UINT32_MAX;
        uint32_t batchId = UINT32_MAX;
        uint32_t srcRank = UINT32_MAX;
        uint32_t dstRank = UINT32_MAX;
        uint32_t opId = UINT32_MAX;
        uint64_t correlationId = UINT64_MAX;
        uint64_t opName = UINT64_MAX;
        uint64_t taskType = UINT64_MAX;
        uint64_t groupName = UINT64_MAX;
        uint64_t transportType = UINT64_MAX;
        uint64_t size = UINT64_MAX;
        uint64_t dataType = UINT64_MAX;
        uint64_t linkType = UINT64_MAX;
        uint64_t notifyId = UINT64_MAX;
        uint64_t rdmaType = UINT64_MAX;
    };
    struct HcclSingleDeviceData {
        uint32_t connectionId = UINT32_MAX;
        double timestamp = 0.0;
        double duration = 0.0;
        std::string opName;
        std::string HCCLName;
        std::string groupName;
        std::string transportType;
        std::string dataType;
        std::string linkType;
        std::string rdmaType;
    };
public:
    CommunicationInfoProcessor() = default;
    CommunicationInfoProcessor(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    virtual ~CommunicationInfoProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriDataFormat GetData(const DBInfo &hcclSingleDeviceDB);
    bool FormatData(const OriDataFormat &oriData, CommunicationTaskDataFormat &taskData,
                    CommunicationOpDataFormat &opData, const ThreadData &threadData);
    void Update(const HcclFormat &oriData, HcclSingleDeviceData &hcclData, CommunicationTaskData &taskData,
                uint16_t deviceId);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_COMMUNICATION_INFO_PROCESSOR_H
