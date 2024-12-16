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
    // name, globalTaskId, taskType, planeId, groupName, notifyId, rdmaType, srcRank, dstRank, transportType,
    // size, dataType, linkType, opId
    using CommunicationTaskDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint32_t, uint64_t,
                                                               uint64_t, uint64_t, uint32_t, uint32_t, uint64_t,
                                                               uint64_t, uint64_t, uint64_t, uint32_t>>;
    // opName, start, end, connectionId, group_name, opId, relay, retry, data_type, alg_type, count, op_type
    using CommunicationOpDataFormat = std::vector<std::tuple<uint64_t, uint64_t, uint64_t, uint64_t, uint64_t,
                                                             uint64_t, int32_t, int32_t, uint64_t, uint64_t,
                                                             uint64_t, uint64_t>>;
    CommunicationInfoProcessor() = default;
    CommunicationInfoProcessor(const std::string &msprofDBPath, const std::set<std::string> &profPaths);
    virtual ~CommunicationInfoProcessor() = default;
    bool Run() override;
protected:
    bool Process(const std::string &fileDir) override;
private:
    template<typename T> static bool ConvertTaskData(CommunicationTaskDataFormat &taskData,
                                                     const std::vector<T> &commTask);
    template<typename T> static void ConvertOpData(CommunicationOpDataFormat &opData, const std::vector<T> &commOp,
                                                   const std::string &fileDir);
};

} // Database
} // Viewer
} // Analysis

#endif // ANALYSIS_VIEWER_DATABASE_COMMUNICATION_INFO_PROCESSOR_H
