/* ******************************************************************************
    版权所有 (c) 华为技术有限公司 2025-2025
    Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_track_cache.cpp
 * Description        : cann侧解析建树
 * Author             : msprof team
 * Creation Date      : 2025/05/14
 * *****************************************************************************
 */

#include "cann_track_cache.h"

#include <functional>
#include "securec.h"
#include "cann_hash_cache.h"
#include "communication_calculator.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "activity/activity_manager.h"

namespace Mspti {
namespace Parser {
inline bool InApiRange(const MsprofApi &data, const MsprofCompactInfo &runtimeTrack)
{
    return runtimeTrack.timeStamp <= data.endTime && data.beginTime <= runtimeTrack.timeStamp;
}

inline bool InApiRange(const MsprofApi &data1, const MsprofApi &data2)
{
    return data1.beginTime <= data2.beginTime && data2.endTime <= data1.endTime;
}

CannTrackCache::~CannTrackCache()
{
    StopTask();
}

msptiResult CannTrackCache::StartTask()
{
    if (!t_.joinable()) {
        threadRun_.store(true);
        t_ = std::thread(std::bind(&CannTrackCache::Run, this));
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::StopTask()
{
    threadRun_.store(false);
    cv.notify_one();
    if (t_.joinable()) {
        t_.join();
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::Run()
{
    pthread_setname_np(pthread_self(), "CannTrackCache");
    while (threadRun_) {
        std::unique_lock<std::mutex> lock(cvMutex_);
        cv.wait(lock, [this] { return !targetThreadId_.empty() || !threadRun_.load(); });
        if (!threadRun_.load()) {
            break;
        }

        std::vector<uint64_t> threads;
        {
            std::lock_guard<std::mutex> lk(targetThreadMutex_);
            threads = std::move(targetThreadId_);
            targetThreadId_.clear();
        }

        if (threads.empty()) {
            continue;
        }

        for (auto threadId : threads) {
            auto cache = GetOrCreateCache(threadId);
            Analysis(cache);
        }
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::AppendTsTrack(const MsprofCompactInfo *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto cache = GetOrCreateCache(data->threadId);
    if (cache == nullptr) {
        MSPTI_LOGE("copy MsprofCompactInfo data failed");
        return MSPTI_ERROR_INNER;
    }
    std::shared_ptr<MsprofCompactInfo> copy;
    Mspti::Common::MsptiMakeSharedPtr(copy);
    if (copy == nullptr) {
        MSPTI_LOGE("copy MsprofCompactInfo data failed");
        return MSPTI_ERROR_INNER;
    }
    errno_t res = memcpy_s(copy.get(), sizeof(MsprofCompactInfo), data, sizeof(MsprofCompactInfo));
    if (res != EOK) {
        MSPTI_LOGE("memcpy MsprofCompactInfo failed! threadId is %d", data->threadId);
        return MSPTI_ERROR_INNER;
    }
    cache->taskQueue.Push(copy);
    return MSPTI_SUCCESS;
}

// Queue的push pop的多线程放最后考虑
msptiResult CannTrackCache::AppendNodeLunch(const MsprofApi *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto cache = GetOrCreateCache(data->threadId);
    if (cache == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    {
        std::shared_ptr<MsprofApi> copy;
        Mspti::Common::MsptiMakeSharedPtr(copy);
        if (copy == nullptr) {
            MSPTI_LOGE("copy NodeLunch MsprofApi data failed");
            return MSPTI_ERROR_INNER;
        }
        errno_t res = memcpy_s(copy.get(), sizeof(MsprofApi), data, sizeof(MsprofApi));
        if (res != EOK) {
            MSPTI_LOGE("memcpy nodeLaunch MsprofApi failed! threadId is %d", data->threadId);
            return MSPTI_ERROR_INNER;
        }
        cache->nodeLaunchQueue.Push(copy);
    }

    {
        std::lock_guard<std::mutex> lk(targetThreadMutex_);
        targetThreadId_.push_back(data->threadId);
    }

    {
        std::lock_guard<std::mutex> lock(cvMutex_);
        cv.notify_one(); // 通知消费者有新数据
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::AppendCommunication(const MsprofApi *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto cache = GetOrCreateCache(data->threadId);
    if (cache == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    std::shared_ptr<MsprofApi> copy;
    Mspti::Common::MsptiMakeSharedPtr(copy);
    if (copy == nullptr) {
        MSPTI_LOGE("copy Communication MsprofApi data failed");
        return MSPTI_ERROR_INNER;
    }
    errno_t res = memcpy_s(copy.get(), sizeof(MsprofApi), data, sizeof(MsprofApi));
    if (res != EOK) {
        MSPTI_LOGE("memcpy nodeLaunch MsprofApi failed! threadId is %d", data->threadId);
        return MSPTI_ERROR_INNER;
    }
    cache->communicationQueue.Push(copy);
    return MSPTI_SUCCESS;
}

bool CannTrackCache::IsCommunicationNode(const MsprofApi &nodeLaunchApi, const CannThreadCachePtr &cache)
{
    if (cache->communicationQueue.IsEmpty()) {
        return false;
    }
    std::shared_ptr<MsprofApi> communication;
    cache->communicationQueue.Peek(communication);
    return InApiRange(nodeLaunchApi, *communication);
}

msptiResult CannTrackCache::Analysis(const CannThreadCachePtr &cache)
{
    while (!cache->nodeLaunchQueue.IsEmpty()) {
        std::shared_ptr<MsprofApi> nodeLaunchApi;
        cache->nodeLaunchQueue.Pop(nodeLaunchApi);

        std::shared_ptr<ApiEvent> api2TaskInfo;
        Mspti::Common::MsptiMakeSharedPtr(api2TaskInfo);
        if (!UNLIKELY(api2TaskInfo)) {
            MSPTI_LOGE("fail to malloc api2TaskInfo");
            return MSPTI_ERROR_INNER;
        }
        api2TaskInfo->api = *nodeLaunchApi;
        api2TaskInfo->threadId = nodeLaunchApi->threadId;
        api2TaskInfo->level = nodeLaunchApi->level;
        if (IsCommunicationNode(*nodeLaunchApi, cache)) {
            MountCommunicationNode(*api2TaskInfo, cache);
            CommunicationCalculator::GetInstance().AppendApi2TaskInfo(api2TaskInfo);
        } else {
            MountCompactInfo(*api2TaskInfo, cache);
        }
    }
    return MSPTI_SUCCESS;
}

void CannTrackCache::MountCommunicationNode(ApiEvent &apiEvent, const CannThreadCachePtr &cache)
{
    std::shared_ptr<MsprofApi> frontApi;
    while (!cache->communicationQueue.IsEmpty()) {
        cache->communicationQueue.Peek(frontApi);
        if (!InApiRange(apiEvent.api, *frontApi)) {
            return;
        }
        std::shared_ptr<ApiEvent> communicationTask;
        Mspti::Common::MsptiMakeSharedPtr(communicationTask);
        if (!UNLIKELY(communicationTask)) {
            MSPTI_LOGE("fail to malloc communicationTask");
            return;
        }
        communicationTask->threadId = apiEvent.threadId;
        communicationTask->api = *frontApi;
        communicationTask->level = communicationTask->api.level;
        MountCompactInfo(*communicationTask, cache);
        apiEvent.childs.emplace_back(communicationTask);
        cache->communicationQueue.Pop(frontApi);
    }
}

void CannTrackCache::MountCompactInfo(ApiEvent &apiEvent, const CannThreadCachePtr &cache)
{
    std::shared_ptr<MsprofCompactInfo> curTask;
    auto res = cache->taskQueue.PopIf(curTask, [&apiEvent](const std::shared_ptr<MsprofCompactInfo>& item) {
        return InApiRange(apiEvent.api, *item);
    });
    if (res) {
        apiEvent.compactInfo = *curTask;
    }
}

CannThreadCachePtr CannTrackCache::GetOrCreateCache(uint64_t threadId)
{
    std::lock_guard<std::mutex> lock(threadCachesMutex_);
    auto it = threadCaches_.find(threadId);
    if (it != threadCaches_.end()) {
        return it->second;
    }
    std::shared_ptr<CannThreadCache> cache;
    Mspti::Common::MsptiMakeSharedPtr(cache);
    if (cache == nullptr) {
        return nullptr;
    }
    threadCaches_[threadId] = cache;
    return cache;
}
}
}