/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_inject.h
 * Description        : Injection of hccl.
 * Author             : msprof team
 * Creation Date      : 2024/08/17
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_INJECT_HCCL_INJECT_H
#define MSPTI_COMMON_INJECT_HCCL_INJECT_H

#include "csrc/common/inject/inject_base.h"
#include "csrc/include/mspti_result.h"

typedef enum {
    HCCL_REDUCE_SUM = 0,    /**< sum */
    HCCL_REDUCE_PROD = 1,   /**< prod */
    HCCL_REDUCE_MAX = 2,    /**< max */
    HCCL_REDUCE_MIN = 3,    /**< min */
    HCCL_REDUCE_RESERVED    /**< reserved */
} HcclReduceOp;

/**
 * @brief HCCL data type
 */
typedef enum {
    HCCL_DATA_TYPE_INT8 = 0,    /**< int8 */
    HCCL_DATA_TYPE_INT16 = 1,   /**< int16 */
    HCCL_DATA_TYPE_INT32 = 2,   /**< int32 */
    HCCL_DATA_TYPE_FP16 = 3,    /**< fp16 */
    HCCL_DATA_TYPE_FP32 = 4,    /**< fp32 */
    HCCL_DATA_TYPE_INT64 = 5,    /**< int64 */
    HCCL_DATA_TYPE_UINT64 = 6,    /**< uint64 */
    HCCL_DATA_TYPE_UINT8 = 7,    /**< uint8 */
    HCCL_DATA_TYPE_UINT16 = 8,   /**< uint16 */
    HCCL_DATA_TYPE_UINT32 = 9,   /**< uint32 */
    HCCL_DATA_TYPE_FP64 = 10, /**< fp64 */
    HCCL_DATA_TYPE_BFP16 = 11,    /**< bfp16 */
    HCCL_DATA_TYPE_INT128 = 12,   /**< int128 */
    HCCL_DATA_TYPE_RESERVED     /**< reserved */
} HcclDataType;

typedef void *HcclComm;

typedef void *aclrtStream;

typedef enum {
    HCCL_SUCCESS = 0,               /**< success */
    HCCL_E_PARA = 1,                /**< parameter error */
    HCCL_E_PTR = 2,                 /**< empty pointer */
    HCCL_E_MEMORY = 3,              /**< memory error */
    HCCL_E_INTERNAL = 4,            /**< internal error */
    HCCL_E_NOT_SUPPORT = 5,         /**< not support feature */
    HCCL_E_NOT_FOUND = 6,           /**< not found specific resource */
    HCCL_E_UNAVAIL = 7,             /**< resource unavailable */
    HCCL_E_SYSCALL = 8,             /**< call system interface error */
    HCCL_E_TIMEOUT = 9,             /**< timeout */
    HCCL_E_OPEN_FILE_FAILURE = 10,  /**< open file fail */
    HCCL_E_TCP_CONNECT = 11,        /**< tcp connect fail */
    HCCL_E_ROCE_CONNECT = 12,       /**< roce connect fail */
    HCCL_E_TCP_TRANSFER = 13,       /**< tcp transfer fail */
    HCCL_E_ROCE_TRANSFER = 14,      /**< roce transfer fail */
    HCCL_E_RUNTIME = 15,            /**< call runtime api fail */
    HCCL_E_DRV = 16,                /**< call driver api fail */
    HCCL_E_PROFILING = 17,          /**< call profiling api fail */
    HCCL_E_CCE = 18,                /**< call cce api fail */
    HCCL_E_NETWORK = 19,            /**< call network api fail */
    HCCL_E_AGAIN = 20,              /**< try again */
    HCCL_E_REMOTE = 21,             /**< error cqe */
    HCCL_E_SUSPENDING = 22,         /**< error communicator suspending */
    HCCL_E_RESERVED                 /**< reserved */
} HcclResult;

typedef enum {
    HCCL_SEND = 0,
    HCCL_RECV = 1,
    HCCL_SEND_RECV_RESERVED
} HcclSendRecvType;

typedef struct HcclSendRecvItemDef {
    HcclSendRecvType sendRecvType;
    void *buf;
    uint64_t count;
    HcclDataType dataType;
    uint32_t remoteRank;
} HcclSendRecvItem;

#if defined(__cplusplus)
extern "C" {
#endif

MSPTI_API HcclResult HcclAllReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType,
                                   HcclReduceOp op, HcclComm comm, aclrtStream stream);
MSPTI_API HcclResult HcclBroadcast(VOID_PTR buf, uint64_t count, HcclDataType dataType, uint32_t root, HcclComm comm,
                                   aclrtStream stream);
MSPTI_API HcclResult HcclAllGather(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t sendCount, HcclDataType dataType,
                                   HcclComm comm, aclrtStream stream);
MSPTI_API HcclResult HcclReduceScatter(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t recvCount, HcclDataType dataType,
                                       HcclReduceOp op, HcclComm comm, aclrtStream stream);
MSPTI_API HcclResult HcclReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType,
                                HcclReduceOp op, uint32_t root, HcclComm comm, aclrtStream stream);
MSPTI_API HcclResult HcclAlltoAll(const VOID_PTR sendBuf, uint64_t sendCount, HcclDataType sendType,
                                  const VOID_PTR recvBuf, uint64_t recvCount, HcclDataType recvType, HcclComm comm,
                                  aclrtStream stream);
MSPTI_API HcclResult HcclAlltoAllV(const VOID_PTR sendBuf, const VOID_PTR sendCounts, const VOID_PTR sdispls,
                                   HcclDataType sendType, const VOID_PTR recvBuf, const VOID_PTR recvCounts,
                                   const VOID_PTR rdispls, HcclDataType recvType, HcclComm comm, aclrtStream stream);
MSPTI_API HcclResult HcclBarrier(HcclComm comm, aclrtStream stream);
MSPTI_API HcclResult HcclScatter(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t recvCount, HcclDataType dataType,
                                 uint32_t root, HcclComm comm, aclrtStream stream);
MSPTI_API HcclResult HcclSend(VOID_PTR sendBuf, uint64_t count, HcclDataType dataType, uint32_t destRank, HcclComm comm,
                              aclrtStream stream);
MSPTI_API HcclResult HcclRecv(VOID_PTR recvBuf, uint64_t count, HcclDataType dataType, uint32_t srcRank, HcclComm comm,
                              aclrtStream stream);
MSPTI_API HcclResult HcclBatchSendRecv(HcclSendRecvItem* sendRecvInfo, uint32_t itemNum, HcclComm comm,
                                       aclrtStream stream);
// Inner
HcclResult HcclGetCommName(HcclComm comm, char* commName);
HcclResult HcclGetRankSize(HcclComm comm, uint32_t *rankSize);

#if defined(__cplusplus)
}
#endif

#endif // MSPTI_PROJECT_HCCL_INJECT_H
