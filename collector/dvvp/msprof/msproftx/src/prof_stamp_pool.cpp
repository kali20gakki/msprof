/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: msprof tx stamp pool
 * Author:
 * Create: 2021-11-22
 */
#include "prof_stamp_pool.h"

#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "utils.h"
using namespace analysis::dvvp::common::error;

namespace Msprof {
namespace MsprofTx {
using MsprofStampCtrlHandle = struct MsprofStampCtrlHandle;

MsprofStampCtrlHandle* g_stampPoolHandle = nullptr;

MsprofStampPool::MsprofStampPool()
{
}

MsprofStampPool::~MsprofStampPool()
{
}

int MsprofStampPool::Init(int size)
{
    if (size > MAX_STAMP_SIZE || size <= 0) {
        MSPROF_LOGE("Init Stamp Pool Failed, Invalid input size %d.", size);
        return PROFILING_FAILED;
    }
    if (g_stampPoolHandle != nullptr) {
        return PROFILING_SUCCESS;
    }

    MSPROF_LOGI("Init Stamp Pool, Input Size:%d", size);
    singleTStack_.reserve(size);
    g_stampPoolHandle = new (std::nothrow) MsprofStampCtrlHandle();
    FUNRET_CHECK_EQUAL_RET_VALUE(g_stampPoolHandle, nullptr, PROFILING_FAILED);
    g_stampPoolHandle->freeCnt = static_cast<uint32_t>(size);
    g_stampPoolHandle->usedCnt = 0;

    g_stampPoolHandle->memPool =
        (struct MsprofStampInstance*) calloc(1, size * sizeof(struct MsprofStampInstance));
    if (g_stampPoolHandle->memPool == nullptr) {
        MSPROF_LOGE("Init Stamp Pool Failed, Memory Not Enough.");
        return PROFILING_FAILED;
    }
    g_stampPoolHandle->instanceSize = static_cast<uint32_t>(size);
    g_stampPoolHandle->freelist = g_stampPoolHandle->memPool;

    MsprofStampInstance* node = nullptr;
    for (int i = 0; i < size; i++) {
        node = g_stampPoolHandle->memPool + i;
        node->id = i;
        if (i == size - 1) {
            node->next = nullptr;
        } else {
            node->next = g_stampPoolHandle->memPool + (i + 1);
        }
    }
    return PROFILING_SUCCESS;
}

int MsprofStampPool::UnInit() const
{
    if (g_stampPoolHandle == nullptr) {
        return PROFILING_SUCCESS;
    }

    if (g_stampPoolHandle->usedCnt != 0) {
        /* stamp is used, can not free */
        return PROFILING_FAILED;
    }

    free(g_stampPoolHandle->memPool);

    delete g_stampPoolHandle;
    g_stampPoolHandle = nullptr;
    return PROFILING_SUCCESS;
}

MsprofStampInstance* MsprofStampPool::CreateStamp()
{
    std::lock_guard<std::mutex> lk(memoryListMtx_);

    if (g_stampPoolHandle->usedCnt >= g_stampPoolHandle->instanceSize) {
        MSPROF_LOGE("[CreateStamp]Failed to create stamp because the number of used has reached the online!");
        return nullptr;
    }

    /* get freeNode frome freelist's head */
    struct MsprofStampInstance* freeNode = g_stampPoolHandle->freelist;
    g_stampPoolHandle->freelist = freeNode->next;

    /* insert freeNode to usedlist's head */
    struct MsprofStampInstance* usedNode = g_stampPoolHandle->usedlist;
    freeNode->next = usedNode;
    g_stampPoolHandle->usedlist = freeNode;

    /* update cnt info */
    g_stampPoolHandle->usedCnt++;
    g_stampPoolHandle->freeCnt--;

    return freeNode;
}

void MsprofStampPool::DestroyStamp(const MsprofStampInstance* stamp)
{
    std::lock_guard<std::mutex> lk(memoryListMtx_);

    if (g_stampPoolHandle->usedCnt == 0 || stamp == nullptr) {
        return;
    }

    struct MsprofStampInstance* usedNode = g_stampPoolHandle->usedlist;
    struct MsprofStampInstance* prevNode = nullptr;
    bool foundNode = false;

    while (usedNode != nullptr) {
        if (stamp == usedNode) {
            foundNode = true;
            break;
        }
        prevNode = usedNode;
        usedNode = usedNode->next;
    }

    if (!foundNode) {
        MSPROF_LOGE("Not Found Current Stamp, Pls Check Input Stamp.");
        return;
    } else {
        /* found */
        struct MsprofStampInstance* freeNode = g_stampPoolHandle->freelist;
        if (prevNode == nullptr) {
            /* head node */
            g_stampPoolHandle->usedlist = usedNode->next;
        } else {
            prevNode->next = usedNode->next;
        }

        usedNode->next = freeNode;
        g_stampPoolHandle->freelist = usedNode;
        g_stampPoolHandle->usedCnt--;
        g_stampPoolHandle->freeCnt++;
    }
}

int MsprofStampPool::MsprofStampPush(MsprofStampInstance *stamp)
{
    if (stamp == nullptr) {
        return PROFILING_FAILED;
    }
    std::lock_guard<std::mutex> lk(singleTStackMtx_);
    singleTStack_.push_back(stamp);
    return PROFILING_SUCCESS;
}

MsprofStampInstance* MsprofStampPool::MsprofStampPop()
{
    std::lock_guard<std::mutex> lk(singleTStackMtx_);
    if (singleTStack_.size() == 0) {
        MSPROF_LOGE("[MsprofStampPop]stamp pop failed , stack size is 0!");
        return nullptr;
    }

    MsprofStampInstance *stamp = singleTStack_.back();
    singleTStack_.pop_back();
    return stamp;
}

MsprofStampInstance* MsprofStampPool::GetStampById(int id)
{
    std::lock_guard<std::mutex> lk(memoryListMtx_);
    struct MsprofStampInstance* usedNode = g_stampPoolHandle->usedlist;
    bool foundNode = false;

    while (usedNode != nullptr) {
        if (usedNode->id == id) {
            foundNode = true;
            break;
        }
        usedNode = usedNode->next;
    }

    if (!foundNode) {
        MSPROF_LOGE("Get Stamp Failed, Invalid id-%d.", id);
        return nullptr;
    }

    return usedNode;
}

int MsprofStampPool::GetIdByStamp(MsprofStampInstance* stamp) const
{
    if (stamp == nullptr) {
        return PROFILING_FAILED;
    }

    return stamp->id;
}
}
}
