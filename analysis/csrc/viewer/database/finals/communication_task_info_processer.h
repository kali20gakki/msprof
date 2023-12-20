/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication_task_info_processer.h
 * Description        : communication_task_info_processer，处理HCCLSingleDevice表相关数据
 * Author             : msprof team
 * Creation Date      : 2023/12/16
 * *****************************************************************************
 */
#ifndef ANALYSIS_VIEWER_DATABASE_COMMUNICATION_TASK_INFO_PROCESSER_H
#define ANALYSIS_VIEWER_DATABASE_COMMUNICATION_TASK_INFO_PROCESSER_H

#include "table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成COMMUNICATION_TASK_INFO表
class CommunicationTaskInfoProcesser : public TableProcesser {
    // model_id, index_id, op_name, iteration, hccl_name, group_name, first_timestamp, plane_id, timestamp, duration,
    // is_dynamic, task_type, op_type, connection_id, is_master, stream_id, task_id, struct_type, duration_estimated,
    // local_rank, remote_rank, transport_type, size, data_type, link_type, bandwidth, context_id, notify_id, batch_id
    using ORI_DATA_FORMAT = std::vector<std::tuple<uint32_t, int32_t, std::string, uint32_t, std::string, std::string,
                                                   double, uint32_t, double, double, int32_t, std::string, std::string,
                                                   int32_t, int32_t, uint32_t, uint32_t, std::string, double, int32_t,
                                                   int32_t, std::string, uint64_t, std::string, std::string, uint64_t,
                                                   uint32_t, uint64_t, uint32_t>>;
    // name, correlationId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType
    using PROCESSED_DATA_FORMAT = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t, uint64_t,
                                                         uint64_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t,
                                                         uint64_t>>;
public:
    CommunicationTaskInfoProcesser() = default;
    CommunicationTaskInfoProcesser(const std::string &reportDBPath, const std::vector<std::string> &profPaths);
    virtual ~CommunicationTaskInfoProcesser() = default;
protected:
    void Process(const std::string &fileDir) override;
private:
    ORI_DATA_FORMAT GetData(const std::string &dbPath);
    PROCESSED_DATA_FORMAT FormatData(const ORI_DATA_FORMAT &oriData);
    bool SaveData(const PROCESSED_DATA_FORMAT &processedData);
    DBInfo hcclSingleDeviceDB_;
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_COMMUNICATION_TASK_INFO_PROCESSER_H
