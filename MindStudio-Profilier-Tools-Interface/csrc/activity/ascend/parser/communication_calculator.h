/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------
*/

#ifndef MSPTI_PARSER_COMMUNICATION_CALCULATOR_H
#define MSPTI_PARSER_COMMUNICATION_CALCULATOR_H

#include <unordered_map>
#include <vector>
#include "csrc/common/inject/profapi_inject.h"
#include "csrc/common/bound_queue.h"
#include "csrc/activity/ascend/parser/cann_track_cache.h"
#include "csrc/activity/ascend/parser/device_task_calculator.h"
#include "csrc/include/mspti_activity.h"
#include "csrc/activity/ascend/entity/communication_op_desc.h"

namespace Mspti {
namespace Parser {
class CommunicationCalculator {
    // <deviceId, streamId, taskId>
    using DstType = std::tuple<uint16_t, uint16_t, uint16_t>;
public:
    msptiResult AppendCompactInfo(bool agingFlag, const MsprofCompactInfo *data);

    msptiResult AppendApi2TaskInfo(const ApiEvent& api2TaskInfo);

    static CommunicationCalculator &GetInstance();

    void AppendCommunicationTask(ApiEvent& apiEvent);

private:
    msptiResult ReportCommunication(const DstType& dstKey, const std::unique_ptr<CommunicationOpDesc>& hcclOp);

    msptiResult Record(const DeviceTask& taskTime);

    CommunicationCalculator() = default;

private:
    std::mutex hcclTaskMutex_;

    // 通过eventId找communication算子
    std::unordered_map<uint64_t, std::unique_ptr<CommunicationOpDesc>> eventId2Communication_;

    // 记录每个DstType对应的CommunicationId, 以及是否是最后一个task
    std::unordered_map<DstType, std::pair<uint64_t, bool>, Common::TupleHash> communicationTask2Op_;

    std::mutex communicationOpInfoMutex_;
    std::unordered_map<std::uint64_t, std::queue<std::unique_ptr<CommunicationOpDesc>>>
        communicationOpInfoQueue_;
    std::unordered_map<DstType, std::unique_ptr<CommunicationOpDesc>, Common::TupleHash> taskId2AdditionInfo;
};
}
}

#endif // MSPTI_PARSER_COMMUNICATION_CALCULATOR_H
