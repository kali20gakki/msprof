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
#include "hccl_inject.h"

#include <functional>
#include <utility>
#include <string>

#include "callback/callback_manager.h"
#include "common/function_loader.h"
#include "hccl_range_mark.h"
#include "common/context_manager.h"
#include "common/plog_manager.h"

namespace {
enum HcclFunctionIndex {
    FUNC_HCCL_ALL_REDUCE,
    FUNC_HCCL_BROADCAST,
    FUNC_HCCL_ALL_GATHER,
    FUNC_HCCL_REDUCE_SCATTER,
    FUNC_HCCL_REDUCE,
    FUNC_HCCL_ALL_TO_ALL,
    FUNC_HCCL_ALL_TO_ALL_V,
    FUNC_HCCL_BARRIER,
    FUNC_HCCL_SCATTER,
    FUNC_HCCL_SEND,
    FUNC_HCCL_RECV,
    FUNC_HCCL_BATCH_SEND_RECV,
    FUNC_HCCL_GET_COMM_NAME,
    FUNC_HCCL_GET_RANK_SIZE,
    FUNC_HCCL_COUNT
};

pthread_once_t g_once;
void* g_hcclFuncArray[FUNC_HCCL_COUNT];

void LoadHcclFunction()
{
    g_hcclFuncArray[FUNC_HCCL_ALL_REDUCE] = Mspti::Common::RegisterFunction("libhccl", "HcclAllReduce");
    g_hcclFuncArray[FUNC_HCCL_BROADCAST] = Mspti::Common::RegisterFunction("libhccl", "HcclBroadcast");
    g_hcclFuncArray[FUNC_HCCL_ALL_GATHER] = Mspti::Common::RegisterFunction("libhccl", "HcclAllGather");
    g_hcclFuncArray[FUNC_HCCL_REDUCE_SCATTER] = Mspti::Common::RegisterFunction("libhccl", "HcclReduceScatter");
    g_hcclFuncArray[FUNC_HCCL_REDUCE] = Mspti::Common::RegisterFunction("libhccl", "HcclReduce");
    g_hcclFuncArray[FUNC_HCCL_ALL_TO_ALL] = Mspti::Common::RegisterFunction("libhccl", "HcclAlltoAll");
    g_hcclFuncArray[FUNC_HCCL_ALL_TO_ALL_V] = Mspti::Common::RegisterFunction("libhccl", "HcclAlltoAllV");
    g_hcclFuncArray[FUNC_HCCL_BARRIER] = Mspti::Common::RegisterFunction("libhccl", "HcclBarrier");
    g_hcclFuncArray[FUNC_HCCL_SCATTER] = Mspti::Common::RegisterFunction("libhccl", "HcclScatter");
    g_hcclFuncArray[FUNC_HCCL_SEND] = Mspti::Common::RegisterFunction("libhccl", "HcclSend");
    g_hcclFuncArray[FUNC_HCCL_RECV] = Mspti::Common::RegisterFunction("libhccl", "HcclRecv");
    g_hcclFuncArray[FUNC_HCCL_BATCH_SEND_RECV] = Mspti::Common::RegisterFunction("libhccl", "HcclBatchSendRecv");
    g_hcclFuncArray[FUNC_HCCL_GET_COMM_NAME] = Mspti::Common::RegisterFunction("libhccl", "HcclGetCommName");
    g_hcclFuncArray[FUNC_HCCL_GET_RANK_SIZE] = Mspti::Common::RegisterFunction("libhccl", "HcclGetRankSize");
}
}

HcclResult HcclAllReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType, HcclReduceOp op,
                         HcclComm comm, aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_ALL_REDUCE];
    using HcclAllReduceFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp,
                                                       HcclComm, aclrtStream)>;
    HcclAllReduceFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, VOID_PTR, uint64_t,
        HcclDataType, HcclReduceOp, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, HcclComm,
        aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<P2pOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = count;
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALLREDUCE, __FUNCTION__);
    return func(sendBuf, recvBuf, count, dataType, op, comm, stream);
}

HcclResult HcclBroadcast(VOID_PTR buf, uint64_t count, HcclDataType dataType, uint32_t root, HcclComm comm,
                         aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_BROADCAST];
    using HcclBroadcastFunc = std::function<HcclResult(VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
        aclrtStream)>;
    HcclBroadcastFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, uint64_t, HcclDataType,
        uint32_t, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<P2pOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = count;
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_BROADCAST, __FUNCTION__);
    return func(buf, count, dataType, root, comm, stream);
}

HcclResult HcclAllGather(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t sendCount, HcclDataType dataType, HcclComm comm,
                         aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_ALL_GATHER];
    using HcclAllGatherFunc =
            std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclComm, aclrtStream)>;
    HcclAllGatherFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, VOID_PTR, uint64_t,
        HcclDataType, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<CollectiveOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = sendCount;
    HcclGetRankSize(comm, static_cast<uint32_t *>(&hcclOpDesc->rankSize));
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALLGATHER, __FUNCTION__);
    return func(sendBuf, recvBuf, sendCount, dataType, comm, stream);
}

HcclResult HcclReduceScatter(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t recvCount, HcclDataType dataType,
                             HcclReduceOp op, HcclComm comm, aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_REDUCE_SCATTER];
    using HcclReduceScatterFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp,
                                                           HcclComm, aclrtStream)>;
    HcclReduceScatterFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, VOID_PTR, uint64_t,
        HcclDataType, HcclReduceOp, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<CollectiveOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = recvCount;
    HcclGetRankSize(comm, static_cast<uint32_t *>(&hcclOpDesc->rankSize));
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_REDUCE_SCATTER, __FUNCTION__);
    return func(sendBuf, recvBuf, recvCount, dataType, op, comm, stream);
}

HcclResult HcclReduce(VOID_PTR sendBuf, VOID_PTR recvBuf, uint64_t count, HcclDataType dataType, HcclReduceOp op,
                      uint32_t root, HcclComm comm, aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_REDUCE];
    using HcclReduceFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, uint32_t,
        HcclComm, aclrtStream)>;
    HcclReduceFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, VOID_PTR, uint64_t, HcclDataType,
        HcclReduceOp, uint32_t, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, HcclReduceOp, uint32_t,
            HcclComm, aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<P2pOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = count;
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_REDUCE, __FUNCTION__);
    return func(sendBuf, recvBuf, count, dataType, op, root, comm, stream);
}

HcclResult HcclAlltoAll(const VOID_PTR sendBuf, uint64_t sendCount, HcclDataType sendType, const VOID_PTR recvBuf,
                        uint64_t recvCount, HcclDataType recvType, HcclComm comm, aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_ALL_TO_ALL];
    using HcclAlltoAllFunc = std::function<HcclResult(const void*, uint64_t, HcclDataType, const void*, uint64_t,
                                                      HcclDataType, HcclComm, aclrtStream)>;
    HcclAlltoAllFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(const void*, uint64_t, HcclDataType,
        const void*, uint64_t, HcclDataType, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, const void*, uint64_t, HcclDataType, const void*, uint64_t,
            HcclDataType, HcclComm, aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<CollectiveOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = sendType;
    hcclOpDesc->count = sendCount;
    HcclGetRankSize(comm, static_cast<uint32_t *>(&hcclOpDesc->rankSize));
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALL_TO_ALL, __FUNCTION__);
    return func(sendBuf, sendCount, sendType, recvBuf, recvCount, recvType, comm, stream);
}

HcclResult HcclAlltoAllV(const VOID_PTR sendBuf, const VOID_PTR sendCounts, const VOID_PTR sdispls,
                         HcclDataType sendType, const VOID_PTR recvBuf, const VOID_PTR recvCounts,
                         const VOID_PTR rdispls, HcclDataType recvType, HcclComm comm, aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_ALL_TO_ALL_V];
    using HcclAlltoAllVFunc = std::function<HcclResult(const void*, const void*, const void*, HcclDataType,
        const void*, const void*, const void*, HcclDataType, HcclComm, aclrtStream)>;
    HcclAlltoAllVFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(const void*, const void*, const void*,
        HcclDataType, const void*, const void*, const void*, HcclDataType, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, const void*, const void*, const void*, HcclDataType,
            const void*, const void*, const void*, HcclDataType, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<All2AllVOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->sendType = sendType;
    hcclOpDesc->sendCounts = sendCounts;
    hcclOpDesc->recvType = recvType;
    hcclOpDesc->recvCounts = recvCounts;
    HcclGetRankSize(comm, static_cast<uint32_t *>(&hcclOpDesc->rankSize));
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_ALL_TO_ALLV, __FUNCTION__);
    return func(sendBuf, sendCounts, sdispls, sendType, recvBuf, recvCounts, rdispls, recvType, comm, stream);
}

HcclResult HcclBarrier(HcclComm comm, aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_BARRIER];
    using HcclBarrierFunc = std::function<HcclResult(HcclComm, aclrtStream)>;
    HcclBarrierFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(HcclComm, aclrtStream)>(voidFunc);
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
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_SCATTER];
    using HcclScatterFunc = std::function<HcclResult(VOID_PTR, VOID_PTR, uint64_t, HcclDataType, uint32_t,
                                                            HcclComm, aclrtStream)>;
    HcclScatterFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, VOID_PTR, uint64_t, HcclDataType,
        uint32_t, HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, VOID_PTR, uint64_t, HcclDataType, uint32_t,
            HcclComm, aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<CollectiveOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = recvCount;
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_SCATTER, __FUNCTION__);
    return func(sendBuf, recvBuf, recvCount, dataType, root, comm, stream);
}

HcclResult HcclSend(VOID_PTR  sendBuf, uint64_t count, HcclDataType dataType, uint32_t destRank, HcclComm comm,
                    aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_SEND];
    using HcclSendFunc = std::function<HcclResult(VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm, aclrtStream)>;
    HcclSendFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, uint64_t, HcclDataType, uint32_t,
        HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<P2pOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = count;
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_SEND, __FUNCTION__);
    return func(sendBuf, count, dataType, destRank, comm, stream);
}

HcclResult HcclRecv(VOID_PTR  recvBuf, uint64_t count, HcclDataType dataType, uint32_t srcRank, HcclComm comm,
                    aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_RECV];
    using HcclRecvFunc = std::function<HcclResult(VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm, aclrtStream)>;
    HcclRecvFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(VOID_PTR, uint64_t, HcclDataType, uint32_t,
        HcclComm, aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, VOID_PTR, uint64_t, HcclDataType, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");

    std::shared_ptr<P2pOpDesc> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    hcclOpDesc->dataType = dataType;
    hcclOpDesc->count = count;
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);

    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_RECV, __FUNCTION__);
    return func(recvBuf, count, dataType, srcRank, comm, stream);
}

HcclResult HcclBatchSendRecv(HcclSendRecvItem* sendRecvInfo, uint32_t itemNum, HcclComm comm, aclrtStream stream)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_BATCH_SEND_RECV];
    using HcclBatchSendRecvFunc = std::function<HcclResult(HcclSendRecvItem*, uint32_t, HcclComm, aclrtStream)>;
    HcclBatchSendRecvFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(HcclSendRecvItem*, uint32_t, HcclComm,
        aclrtStream)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, HcclSendRecvItem*, uint32_t, HcclComm,
            aclrtStream>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    std::shared_ptr<BatchSendRecvOp> hcclOpDesc;
    Mspti::Common::MsptiMakeSharedPtr(hcclOpDesc);
    if (sendRecvInfo != nullptr) {
        for (uint32_t i = 0; i < itemNum; i++) {
            P2pOpDesc batchSendRecvItemDesc;
            batchSendRecvItemDesc.dataType = sendRecvInfo[i].dataType;
            batchSendRecvItemDesc.count = sendRecvInfo[i].count;
            hcclOpDesc->batchSendRecvItem.emplace_back(batchSendRecvItemDesc);
        }
    } else {
        MSPTI_LOGE("op HcclBatchSendRecv, sendRecvInfo is nullptr");
    }
    hcclOpDesc->opName = __FUNCTION__;
    auto marker = Mspti::Parser::HcclMarkFactory::createMarker(stream, comm, hcclOpDesc);
    Mspti::Callback::CallbackScope scope(MSPTI_CB_DOMAIN_HCCL, MSPTI_CBID_HCCL_SENDRECV, __FUNCTION__);
    return func(sendRecvInfo, itemNum, comm, stream);
}

HcclResult HcclGetCommName(HcclComm comm, char* commName)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_GET_COMM_NAME];
    using HcclGetCommNameFunc = std::function<HcclResult(HcclComm, char*)>;
    HcclGetCommNameFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(HcclComm, char*)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, HcclComm, char*>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    return func(comm, commName);
}

HcclResult HcclGetRankSize(HcclComm comm, uint32_t *rankSize)
{
    pthread_once(&g_once, LoadHcclFunction);
    void* voidFunc = g_hcclFuncArray[FUNC_HCCL_GET_RANK_SIZE];
    using HcclGetRankSizeFunc = std::function<HcclResult(HcclComm comm, uint32_t *rankSize)>;
    HcclGetRankSizeFunc func = Mspti::Common::ReinterpretConvert<HcclResult (*)(HcclComm comm,
        uint32_t *rankSize)>(voidFunc);
    if (func == nullptr) {
        Mspti::Common::GetFunction<HcclResult, HcclComm, uint32_t *>("libhccl", __FUNCTION__, func);
    }
    THROW_FUNC_NOTFOUND(func, __FUNCTION__, "libhccl.so");
    return func(comm, rankSize);
}