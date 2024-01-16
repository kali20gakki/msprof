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

#include "analysis/csrc/viewer/database/finals/table_processer.h"

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于生成COMMUNICATION_TASK_INFO表
class CommunicationTaskInfoProcesser : public TableProcesser {
    // model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, remote_rank,
    // transport_type, size, data_type, link_type, context_id, notify_id, batch_id, rdma_type
    using OriDataFormat = std::vector<std::tuple<uint32_t, std::string, std::string, std::string, int32_t,
                                                   uint64_t, uint32_t, uint32_t, uint32_t, std::string, uint64_t,
                                                   std::string, std::string, uint32_t, uint64_t, uint32_t,
                                                   std::string>>;
    // name, correlationId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType
    using ProcessedDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t, uint64_t,
                                                         uint64_t, uint32_t, uint32_t, uint64_t, uint64_t, uint64_t,
                                                         uint64_t>>;
public:
    CommunicationTaskInfoProcesser() = default;
    CommunicationTaskInfoProcesser(const std::string &reportDBPath, const std::set<std::string> &profPaths);
    virtual ~CommunicationTaskInfoProcesser() = default;
protected:
    bool Process(const std::string &fileDir) override;
private:
    OriDataFormat GetData();
    static ProcessedDataFormat FormatData(const OriDataFormat &oriData);
    DBInfo hcclSingleDeviceDB_;
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_COMMUNICATION_TASK_INFO_PROCESSER_H
