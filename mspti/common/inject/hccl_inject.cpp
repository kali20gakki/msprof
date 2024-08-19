/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : hccl_inject.cpp
 * Description        : Injection of hccl.
 * Author             : msprof team
 * Creation Date      : 2024/08/17
 * *****************************************************************************
*/
#include <functional>

#include "callback/callback_manager.h"
#include "common/function_loader.h"
#include "hccl_inject.h"

class HcclInject {
public:
    HcclInject() noexcept
    {
        Mspti::Common::RegisterFunction("libhccl", "HcclAllReduce"),
        Mspti::Common::RegisterFunction("libhccl", "HcclBroadcast"),
        Mspti::Common::RegisterFunction("libhccl", "HcclAllGather"),
        Mspti::Common::RegisterFunction("libhccl", "HcclReduceScatter"),
        Mspti::Common::RegisterFunction("libhccl", "HcclReduce"),
        Mspti::Common::RegisterFunction("libhccl", "HcclAlltoAll"),
        Mspti::Common::RegisterFunction("libhccl", "HcclAlltoAllV"),
        Mspti::Common::RegisterFunction("libhccl", "HcclBarrier"),
        Mspti::Common::RegisterFunction("libhccl", "HcclScatter"),
        Mspti::Common::RegisterFunction("libhccl", "HcclSend"),
        Mspti::Common::RegisterFunction("libhccl", "HcclRecv"),
        Mspti::Common::RegisterFunction("libhccl", "HcclBatchSendRecv");
    };
    ~HcclInject() = default;
};

HcclInject g_hcclInject;

HcclResult HcclAllReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType, HcclReduceOp op,
                         HcclComm comm, aclrtStream stream)
{
    using HcclAllReduceFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp,
                                                       HcclComm, aclrtStream)>;
    static HcclAllReduceFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, HcclComm,
        aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALLREDUCE, __FUNCTION__);
    return func(sendBuf, recvBuf, count, dataType, op, comm, stream);
}

HcclResult HcclBroadcast(VOID_PTR buf, uint64_t count, HcclDataType dataType, uint32_t root, HcclComm comm,
                         aclrtStream stream)
{
    using HcclBroadcastFunc = std::function<HcclResult(VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
        aclrtStream)>;
    static HcclBroadcastFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_BROADCAST, __FUNCTION__);
    return func(buf, count, dataType, root, comm, stream);
}

HcclResult HcclAllGather(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t sendCount, HcclDataType dataType, HcclComm comm,
                         aclrtStream stream)
{
    using HcclAllGatherFunc =
            std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclComm, aclrtStream)>;
    static HcclAllGatherFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALLGATHER, __FUNCTION__);
    return func(sendBuf, recvBuf, sendCount, dataType, comm, stream);
}

HcclResult HcclReduceScatter(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t recvCount, HcclDataType dataType,
                             HcclReduceOp op, HcclComm comm, aclrtStream stream)
{
    using HcclReduceScatterFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp,
                                                           HcclComm, aclrtStream)>;
    static HcclReduceScatterFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_REDUCE_SCATTER, __FUNCTION__);
    return func(sendBuf, recvBuf, recvCount, dataType, op, comm, stream);
}

HcclResult HcclReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType, HcclReduceOp op,
                      uint32_t root, HcclComm comm, aclrtStream stream)
{
    using HcclReduceFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, uint32_t,
        HcclComm, aclrtStream)>;
    static HcclReduceFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, uint32_t,
            HcclComm, aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_REDUCE, __FUNCTION__);
    return func(sendBuf, recvBuf, count, dataType, op, root, comm, stream);
}

HcclResult HcclAlltoAll(const VOID_PTR sendBuf, uint64_t sendCount, HcclDataType sendType, const VOID_PTR recvBuf,
                        uint64_t recvCount, HcclDataType recvType, HcclComm comm, aclrtStream stream)
{
    using HcclAlltoAllFunc = std::function<HcclResult(const void*, uint64_t, HcclDataType, const void*, uint64_t,
                                                      HcclDataType, HcclComm, aclrtStream)>;
    static HcclAlltoAllFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, const void*, uint64_t, HcclDataType, const void*, uint64_t,
            HcclDataType, HcclComm, aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALL_TO_ALL, __FUNCTION__);
    return func(sendBuf, sendCount, sendType, recvBuf, recvCount, recvType, comm, stream);
}

HcclResult HcclAlltoAllV(const VOID_PTR sendBuf, const VOID_PTR sendCounts, const VOID_PTR sdispls,
                         HcclDataType sendType, const VOID_PTR recvBuf, const VOID_PTR recvCounts,
                         const VOID_PTR rdispls, HcclDataType recvType, HcclComm comm, aclrtStream stream)
{
    using HcclAlltoAllVFunc = std::function<HcclResult(const void*, const void*, const void*, HcclDataType,
        const void*, const void*, const void*, HcclDataType, HcclComm, aclrtStream)>;
    static HcclAlltoAllVFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, const void*, const void*, const void*, HcclDataType,
            const void*, const void*, const void*, HcclDataType, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALL_TO_ALLV, __FUNCTION__);
    return func(sendBuf, sendCounts, sdispls, sendType, recvBuf, recvCounts, rdispls, recvType, comm, stream);
}

HcclResult HcclBarrier(HcclComm comm, aclrtStream stream)
{
    using HcclBarrierFunc = std::function<HcclResult(HcclComm, aclrtStream)>;
    static HcclBarrierFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, HcclComm, aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_BARRIER, __FUNCTION__);
    return func(comm, stream);
}

HcclResult HcclScatter(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t recvCount, HcclDataType dataType, uint32_t root,
                       HcclComm comm, aclrtStream stream)
{
    using HcclScatterFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, uint32_t,
                                                            HcclComm, aclrtStream)>;
    static HcclScatterFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, uint32_t,
            HcclComm, aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_SCATTER, __FUNCTION__);
    return func(sendBuf, recvBuf, recvCount, dataType, root, comm, stream);
}

HcclResult HcclSend(VOID_PTR  sendBuf, uint64_t count, HcclDataType dataType, uint32_t destRank, HcclComm comm,
                    aclrtStream stream)
{
    using HcclSendFunc = std::function<HcclResult(VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm, aclrtStream)>;
    static HcclSendFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_SEND, __FUNCTION__);
    return func(sendBuf, count, dataType, destRank, comm, stream);
}

HcclResult HcclRecv(VOID_PTR  recvBuf, uint64_t count, HcclDataType dataType, uint32_t srcRank, HcclComm comm,
                    aclrtStream stream)
{
    using HcclRecvFunc = std::function<HcclResult(VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm, aclrtStream)>;
    static HcclRecvFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_RECV, __FUNCTION__);
    return func(recvBuf, count, dataType, srcRank, comm, stream);
}

HcclResult HcclBatchSendRecv(HcclSendRecvItem* sendRecvInfo, uint32_t itemNum, HcclComm comm, aclrtStream stream)
{
    using HcclBatchSendRecvFunc = std::function<HcclResult(HcclSendRecvItem*, uint32_t, HcclComm, aclrtStream)>;
    static HcclBatchSendRecvFunc func = nullptr;
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, HcclSendRecvItem*, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_SENDRECV, __FUNCTION__);
    return func(sendRecvInfo, itemNum, comm, stream);
}