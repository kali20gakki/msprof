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

namespace Analysis {
namespace Viewer {
namespace Database {
// 该类用于依据HCCLSingelDevice库生成COMMUNICATION_TASK_INFO(通信小算子)和COMMUNICATION_OP表(通信大算子)
class CommunicationInfoProcessor : public TableProcessor {
public:
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
    using KfcOpFormat = std::tuple<uint32_t, std::string, int32_t, int32_t, std::string, std::string, uint64_t,
                                   std::string, std::string, double, double>;
    using OriOpDataFormat = std::vector<HcclOpFormat>;
    using OriKfcOpDataFormat = std::vector<KfcOpFormat>;
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
                                                               uint64_t, uint64_t, uint32_t, uint32_t, uint64_t,
                                                               uint64_t, uint64_t, uint64_t, uint32_t>>;
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                                             uint64_t, int32_t, int32_t, uint64_t, uint64_t,
                                                             uint64_t, uint64_t>>;
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
        uint64_t globalTaskId = UINT64_MAX;
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
    struct CommunicationOpData {
        uint64_t opName = UINT64_MAX;
        uint64_t groupName = UINT64_MAX;
        uint64_t connectionId = UINT64_MAX;
        uint32_t opId = UINT32_MAX;
        uint64_t start = UINT64_MAX;
        uint64_t end = UINT64_MAX;
        int32_t relay = 0;
        int32_t retry = 0;
        uint64_t dataType = UINT64_MAX;
        uint64_t algType = UINT64_MAX;
        uint64_t count = UINT64_MAX;
        uint64_t opType = UINT64_MAX;
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
public:
    CommunicationInfoProcessor() = default;
    CommunicationInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~CommunicationInfoProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    static OriTaskDataFormat GetTaskData(const DBInfo &hcclSingleDeviceDB);
    static OriOpDataFormat GetOpData(const DBInfo &hcclSingleDeviceDB);
    static OriTaskDataFormat GetKfcTaskData(const DBInfo &kfcTask);
    static OriKfcOpDataFormat GetKfcOpData(const DBInfo &kfcOp);
    bool ProcessOneDeviceHccl(const std::string &devicePath, DBInfo &taskDBInfo, DBInfo &opDBInfo,
                              OriTaskDataFormat &oriTaskData, OriOpDataFormat &oriOpData);
    bool ProcessOneDeviceKfc(const std::string &devicePath, DBInfo &kfcTaskDBInfo, DBInfo &kfcOpDBInfo,
                             OriTaskDataFormat &oriKfcTaskData, OriKfcOpDataFormat &oriKfcOpData);
    void FormatData(const OriTaskDataFormat &oriTaskData, const OriOpDataFormat &oriOpData,
                    CommunicationTaskDataFormat &taskData, CommunicationOpDataFormat &opData,
                    const ThreadData &threadData, GeHashMap &hashMap);
    void FormatKfcTaskData(const OriTaskDataFormat &oriTaskData,
                           CommunicationTaskDataFormat &processedTaskData,
                           const ThreadData &threadData, GeHashMap &hashMap);
    void FormatKfcOpData(const OriKfcOpDataFormat &oriOpData,
                         CommunicationOpDataFormat &processedOpData,
                         const ThreadData &threadData, GeHashMap &hashMap);
    void Update(const HcclTaskFormat &oriData, HcclTaskSingleDeviceData &hcclData, CommunicationTaskData &taskData,
                uint16_t deviceId, GeHashMap &hashMap);
    void UpdateOpInfo(CommunicationOpData &opData, uint32_t connectionId,
                      const std::unordered_map<uint32_t, size_t> &opInfoIdxMap,
                      const OriOpDataFormat &oriOpData, GeHashMap &hashMap);
    bool SaveCommData(ThreadData &threadData, OriTaskDataFormat &oriTaskData, OriOpDataFormat &oriOpData,
                      OriTaskDataFormat &oriKfcTaskData, OriKfcOpDataFormat &oriKfcOpData,
                      const std::string &fileDir);
    std::unordered_map<uint32_t, size_t> GenOpInfoIdxmap(const OriOpDataFormat &oriOpData);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_COMMUNICATION_INFO_PROCESSOR_H
