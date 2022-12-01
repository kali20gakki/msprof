/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: msprof tx stamp pool
 * Author:
 * Create: 2021-11-22
 */
#include "msprof_stamp_pool.h"
#include "config/config.h"
#include "errno/error_code.h"
#include "mmpa_api.h"
#include "msprof_dlog.h"
#include "msprof_error_manager.h"
#include "utils.h"

using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;

namespace Msprof {
namespace MsprofTx {
using MsprofStampCtrlHandle = struct MsprofStampCtrlHandle;

MsprofStampCtrlHandle* g_stampPoolHandle = nullptr;
MsprofStampInstance* g_stampInstanceAddr[CURRENT_STAMP_SIZE];

MsprofStampPool::MsprofStampPool()
{
}

MsprofStampPool::~MsprofStampPool()
{
    if (g_stampPoolHandle != nullptr) {
        if (g_stampPoolHandle->memPool != nullptr) {
            free(g_stampPoolHandle->memPool);
        }
        delete g_stampPoolHandle;
        g_stampPoolHandle = nullptr;
    }
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
        g_stampInstanceAddr[i] = node;
        if (i == size - 1) {
            node->next = nullptr;
        } else {
            node->next = g_stampPoolHandle->memPool + (i + 1);
        }
        node->stampInfo.processId = static_cast<uint32_t>(MmGetPid());
        node->stampInfo.dataTag = MSPROF_MSPROFTX_DATA_TAG;
        node->stampInfo.magicNumber = static_cast<uint16_t>(MSPROF_DATA_HEAD_MAGIC_NUM);
        node->report.deviceId = DEFAULT_HOST_ID;
        node->report.dataLen = sizeof(node->stampInfo);
        node->report.data = reinterpret_cast<UNSIGNED_CHAR_PTR>(&node->stampInfo);
        strncpy_s(node->report.tag, static_cast<size_t>(MSPROF_ENGINE_MAX_TAG_LEN), "msproftx", strlen("msproftx"));
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

    /* take out freeNode from freelist's head */
    struct MsprofStampInstance* freeNode = g_stampPoolHandle->freelist;
    g_stampPoolHandle->freelist = freeNode->next;

    /* insert freeNode to usedlist's head */
    struct MsprofStampInstance* usedNode = g_stampPoolHandle->usedlist;
    freeNode->next = usedNode;
    if (usedNode != nullptr) {
        usedNode->prev = freeNode;
    }
    g_stampPoolHandle->usedlist = freeNode;

    /* update cnt info */
    g_stampPoolHandle->usedCnt++;
    g_stampPoolHandle->freeCnt--;

    return freeNode;
}

void MsprofStampPool::DestroyStamp(MsprofStampInstance* stamp)
{
    std::lock_guard<std::mutex> lk(memoryListMtx_);

    if (g_stampPoolHandle->usedCnt == 0 || stamp == nullptr) {
        return;
    }

    /* take out stamp node from usedlist */
    if (g_stampPoolHandle->usedlist == stamp) { // the stamp is first node in usedlist
        g_stampPoolHandle->usedlist = stamp->next;
    } else if (stamp->next == nullptr) { // the stamp is last node in usedlist
        stamp->prev->next = nullptr;
    } else { // the stamp is middle node in usedlist
        stamp->prev->next = stamp->next;
        stamp->next->prev = stamp->prev;
    }

    /* insert stamp not to freelist */
    stamp->next = g_stampPoolHandle->freelist;
    g_stampPoolHandle->freelist = stamp;

    /* update cnt info */
    g_stampPoolHandle->usedCnt--;
    g_stampPoolHandle->freeCnt++;
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
    if (id >= CURRENT_STAMP_SIZE || id < 0) {
        return nullptr;
    }

    return g_stampInstanceAddr[id];
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
