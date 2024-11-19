/**
* @file hcc_inject_stub.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
*
*/

#include "common/inject/hccl_inject.h"

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