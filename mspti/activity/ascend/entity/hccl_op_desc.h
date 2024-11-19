/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_op_desc.h
 * Description        : hccl大算子数据
 * Author             : msprof team
 * Creation Date      : 2024/11/1
 * *****************************************************************************
*/
#ifndef MSPTI_PROJECT_HCCL_OP_DESC_H
#define MSPTI_PROJECT_HCCL_OP_DESC_H

#include <cstdint>
#include <vector>
#include "common/inject/hccl_inject.h"
#include "common/config.h"

struct HcclOpDesc {
    std::string opName;
    uint32_t streamId;
    uint32_t deviceId;
    std::string commName;
    uint64_t start;
    uint64_t end;
    double bandWidth; // GB/s
};

struct P2pOpDesc: public HcclOpDesc {
    uint64_t count;
    HcclDataType dataType;
};

struct CollectiveOpDesc: public HcclOpDesc {
    uint32_t rankSize;
    uint64_t count;
    HcclDataType dataType;
};

struct All2AllVOpDesc: public CollectiveOpDesc {
    const void* sendCounts;
    HcclDataType sendType;
    const void* recvCounts;
    HcclDataType recvType;
};

struct BatchSendRecvOp: public P2pOpDesc {
    std::vector<P2pOpDesc> batchSendRecvItem;
};

#endif // MSPTI_PROJECT_HCCL_OP_DESC_H
