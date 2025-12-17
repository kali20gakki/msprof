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
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_ENTITIES_ASCEND_OBJ_H
#define ANALYSIS_ENTITIES_ASCEND_OBJ_H

#include <cstdint>
#include <memory>

#include "collector/inc/toolchain/prof_common.h"

namespace Analysis {
namespace Domain {

#define DEFAULT_CONTEXT_ID 0xffffffff

// 算子类型枚举
enum class OpType {
    OPTYPE_HCCL_BIG = 0,
    OPTYPE_HCCL_SMALL,
    OPTYPE_COMPUTE,
    OPTYPE_COMPUTE_HCCL,
    OPTYPE_RESERVED,
    OPTYPE_INVALID
};

// 计算算子描述信息
struct OpDesc {
    std::shared_ptr<MsprofCompactInfo> nodeDesc = nullptr;
    std::shared_ptr<MsprofCompactInfo> nodeAttr = nullptr;
    std::shared_ptr<ConcatTensorInfo> tensorDesc = nullptr;
    std::shared_ptr<MsprofAdditionalInfo> ctxId = nullptr;
};

// 通信小算子描述信息
struct HcclSmallOpDesc {
    uint32_t ctxId = DEFAULT_CONTEXT_ID;
    uint8_t isMaster = 0;  // 1代表master
    std::shared_ptr<MsprofAdditionalInfo> hcclInfo = nullptr;

    HcclSmallOpDesc(uint32_t ctxId, uint32_t isMaster, const std::shared_ptr<MsprofAdditionalInfo> &hcclInfo)
        : ctxId(ctxId), isMaster(isMaster), hcclInfo(hcclInfo)
    {}
};

// 通信大算子描述信息
struct HcclBigOpDesc {
    uint64_t beginTime = 0;
    uint64_t endTime = 0;
    uint16_t deviceId = 0;
    uint64_t modelId = 0;
    int32_t indexId = 0;
    int64_t connectionId = 0;
    uint32_t thread_id = 0;
    std::shared_ptr<MsprofCompactInfo> nodeDesc = nullptr;
    std::shared_ptr<MsprofCompactInfo> opInfoDesc = nullptr;
    int64_t kfcConnectionId = 0;

    HcclBigOpDesc(uint64_t begin, uint64_t end, uint16_t deviceId, uint64_t modelId, int32_t indexId,
                  int64_t connectionId, uint32_t threadId, const std::shared_ptr<MsprofCompactInfo> &node,
                  const std::shared_ptr<MsprofCompactInfo> &hcclOpDesc, int64_t kfcConnectionId)
        : beginTime(begin), endTime(end), deviceId(deviceId), modelId(modelId), indexId(indexId),
          connectionId(connectionId), thread_id(threadId), nodeDesc(node), opInfoDesc(hcclOpDesc),
          kfcConnectionId(kfcConnectionId)
    {}
};

// 算子统一对外结构体
struct Operator {
    Operator(std::shared_ptr<OpDesc> desc, const uint64_t &name, const OpType &type)
        : opDesc(std::move(desc)), name(name), type(type)
    {}

    Operator(std::shared_ptr<HcclSmallOpDesc> desc, const uint64_t &name, const OpType &type)
        : hcclSmallOpDesc(std::move(desc)), name(name), type(type)
    {}

    Operator(std::shared_ptr<HcclBigOpDesc> desc, const uint64_t &name, const OpType &type)
        : hcclBigOpDesc(std::move(desc)), name(name), type(type)
    {}

    union {
        std::shared_ptr<OpDesc> opDesc;
        std::shared_ptr<HcclSmallOpDesc> hcclSmallOpDesc;
        std::shared_ptr<HcclBigOpDesc> hcclBigOpDesc;
    };

    uint64_t name = 0; // aka item_id
    OpType type = OpType::OPTYPE_INVALID;

    ~Operator()
    {
        if (opDesc) {
            opDesc.~shared_ptr();
        } else if (hcclSmallOpDesc) {
            hcclSmallOpDesc.~shared_ptr();
        } else if (hcclBigOpDesc) {
            hcclBigOpDesc.~shared_ptr();
        }
    }
};

// 用于存储分析树结果，DBDumper将此信息落盘
struct HostTask {
    uint16_t taskId = 0;       // 和采集侧的数据类型不一致，采集侧的高低16位会被分别用作batchId和TaskId
    uint16_t batchId = 0;
    uint16_t deviceId = 0;
    int32_t requestId = 0;
    uint32_t thread_id = 0;
    uint32_t streamId = 0;
    uint32_t contextId = 0;
    int64_t connection_id = 0; // -1 表示该任务无node直连
    uint64_t modelId = 0;
    uint64_t taskType = 0;
    uint64_t timeStamp = 0;
    uint64_t kernelName = 0;
    std::string taskTypeStr;
    std::shared_ptr<Operator> op = nullptr;
};

// 存储GeFusionOpInfo表
struct GeFusionOpInfo {
    uint64_t modelId;
    std::shared_ptr<ProfFusionOpInfo> fusionOpInfo = nullptr;
    GeFusionOpInfo(uint64_t modelId, const std::shared_ptr<ProfFusionOpInfo> &fusionOp)
        : modelId(modelId), fusionOpInfo(fusionOp)
    {}
};

} // namespace Domain
} // namespace Analysis
#endif // ANALYSIS_ENTITIES_ASCEND_OBJ_H