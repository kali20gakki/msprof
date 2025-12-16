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

#include "csrc/common/inject/hccl_inject.h"

HcclResult HcclAllReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType,
                         HcclReduceOp op, HcclComm comm, aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclBroadcast(VOID_PTR buf, uint64_t count, HcclDataType dataType, uint32_t root, HcclComm comm,
                         aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclAllGather(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t sendCount, HcclDataType dataType,
                         HcclComm comm, aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclReduceScatter(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t recvCount, HcclDataType dataType,
                             HcclReduceOp op, HcclComm comm, aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType,
                      HcclReduceOp op, uint32_t root, HcclComm comm, aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclAlltoAll(const VOID_PTR sendBuf, uint64_t sendCount, HcclDataType sendType,
                        const VOID_PTR recvBuf, uint64_t recvCount, HcclDataType recvType, HcclComm comm,
                        aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclAlltoAllV(const VOID_PTR sendBuf, const VOID_PTR sendCounts, const VOID_PTR sdispls,
                         HcclDataType sendType, const VOID_PTR recvBuf, const VOID_PTR recvCounts,
                         const VOID_PTR rdispls, HcclDataType recvType, HcclComm comm, aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclBarrier(HcclComm comm, aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclScatter(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t recvCount, HcclDataType dataType,
                       uint32_t root, HcclComm comm, aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclSend(VOID_PTR sendBuf, uint64_t count, HcclDataType dataType, uint32_t destRank, HcclComm comm,
                    aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclRecv(VOID_PTR recvBuf, uint64_t count, HcclDataType dataType, uint32_t srcRank, HcclComm comm,
                    aclrtStream stream)
{
    return HCCL_SUCCESS;
}
HcclResult HcclBatchSendRecv(HcclSendRecvItem* sendRecvInfo, uint32_t itemNum, HcclComm comm,
                             aclrtStream stream)
{
    return HCCL_SUCCESS;
}