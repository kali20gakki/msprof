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
#ifndef MSPTI_PROJECT_HCCL_OP_DESC_H
#define MSPTI_PROJECT_HCCL_OP_DESC_H

#include <cstdint>
#include <vector>
#include "csrc/common/inject/hccl_inject.h"
#include "csrc/common/config.h"

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
