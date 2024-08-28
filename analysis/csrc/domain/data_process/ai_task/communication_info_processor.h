/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : communication__info_processor.h
 * Description        : communication_info_processor处理HCCLTaskSingleDevice,HCCLOpSingleDevice表相关数据
 * Author             : msprof team
 * Creation Date      : 2024/08/03
 * *****************************************************************************
 */
#ifndef ANALYSIS_DOMAIN_COMMUNICATION_INFO_PROCESSOR_H
#define ANALYSIS_DOMAIN_COMMUNICATION_INFO_PROCESSOR_H

#include "analysis/csrc/domain/data_process/data_processor.h"
#include "analysis/csrc/domain/entities/viewer_data/ai_task/include/communication_info_data.h"

namespace Analysis {
namespace Domain {
using GeHashMap = std::unordered_map<std::string, std::string>;
// model_id, op_name, hccl_name, group_name, plane_id, stream_id, task_id, local_rank, remote_rank,
// transport_type, size, data_type, link_type, context_id, notify_id, batch_id, rdma_type, timestamp, duration,
// connection_id
using HcclTaskFormat = std::tuple<uint32_t, std::string, std::string, std::string, int32_t, uint64_t, uint32_t,
    uint32_t, uint32_t, std::string, uint64_t, std::string, std::string, uint32_t,
    uint64_t, uint32_t, std::string, double, double, uint32_t>;
using OriTaskDataFormat = std::vector<HcclTaskFormat>;
// connection_id, op_name, relay, retry, data_type, alg_type, count, group_name, op_type
using HcclOpFormat = std::tuple<uint32_t, std::string, int32_t, int32_t, std::string, std::string, uint64_t,
    std::string, std::string>;
using OriOpDataFormat = std::vector<HcclOpFormat>;

// 该类用于依据HCCLSingelDevice库生成COMMUNICATION_TASK_INFO(通信小算子)和COMMUNICATION_OP表(通信大算子)
class CommunicationInfoProcessor : public DataProcessor {
public:
    struct CommunicationData {
        std::vector<CommunicationTaskData> resTaskData;
        std::vector<CommunicationOpData> resOpData;
        OriTaskDataFormat oriTaskData;
        OriOpDataFormat oriOpData;
        uint16_t deviceId = UINT16_MAX;
        Utils::ProfTimeRecord timeRecord;
    };
    struct HcclTaskSingleDeviceData {
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
    CommunicationInfoProcessor() = default;
    explicit CommunicationInfoProcessor(const std::string& profPaths);
    virtual ~CommunicationInfoProcessor() = default;
private:
    bool Process(DataInventory& dataInventory) override;
    OriTaskDataFormat LoadTaskData(const DBInfo& hcclSingleDeviceDB);
    OriOpDataFormat LoadOpData(const DBInfo& hcclSingleDeviceDB);
    bool ProcessOneDevice(const std::string& devicePath, CommunicationData& communicationData,
                          DataInventory& dataInventory);
    bool FormatData(std::vector<CommunicationTaskData>& taskFormatData, std::vector<CommunicationOpData>& opFormatData,
                    CommunicationData& communicationData, GeHashMap& hashMap);
    void Update(const HcclTaskFormat& oriData, HcclTaskSingleDeviceData& hcclData, CommunicationTaskData& taskData,
                uint16_t deviceId, GeHashMap& hashMap);
    void UpdateOpInfo(CommunicationOpData& opData, uint32_t connectionId,
                      const std::unordered_map<uint32_t, size_t>& opInfoIdxMap,
                      const OriOpDataFormat& oriOpData, GeHashMap& hashMap);
    std::unordered_map<uint32_t, size_t> GenOpInfoIdxMap(const OriOpDataFormat& oriOpData);
};
} // Domain
} // Analysis

#endif // ANALYSIS_DOMAIN_COMMUNICATION_INFO_PROCESSOR_H
