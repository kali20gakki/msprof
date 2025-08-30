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
#include <unordered_map>
#include "securec.h"
#include "cann_hash_cache.h"
#include "communication_calculator.h"
#include "common/plog_manager.h"
#include "common/utils.h"
#include "common/mpsc_queue.h"
#include "activity/activity_manager.h"

namespace Mspti {
namespace Parser {
namespace {
inline bool InApiRange(const MsprofApi &data, const MsprofCompactInfo &runtimeTrack)
{
    return runtimeTrack.timeStamp <= data.endTime && data.beginTime <= runtimeTrack.timeStamp;
}

inline bool InApiRange(const MsprofApi &data1, const MsprofApi &data2)
{
    return data1.beginTime <= data2.beginTime && data2.endTime <= data1.endTime;
}
}

template <typename T> struct TagAging {
    std::unique_ptr<T> data{};
    bool agingFlag = true;
    explicit TagAging(T &&value) : data(std::make_unique<T>(std::move(value))), agingFlag(true) {}
    explicit TagAging(bool aging, T &&value) : data(std::make_unique<T>(std::move(value))), agingFlag(aging) {}
    explicit TagAging(bool aging, std::unique_ptr<T> &&value) : data(std::move(value)), agingFlag(aging) {}
};

struct CannThreadCache {
    uint32_t threadId{};
    std::mutex taskMutex;
    std::multimap<uint64_t, std::shared_ptr<TagAging<MsprofCompactInfo>>> taskQueue;
    Mspti::Common::MPSCQueue<std::shared_ptr<TagAging<MsprofApi>>> nodeLaunchQueue;
    Mspti::Common::MPSCQueue<std::shared_ptr<TagAging<MsprofApi>>> communicationQueue;
};

using CannThreadCachePtr = std::shared_ptr<CannThreadCache>;

class CannTrackCache::CannTrackCacheImpl {
public:
    explicit CannTrackCacheImpl() = default;
    ~CannTrackCacheImpl();
    msptiResult StartTask();
    msptiResult StopTask();
    msptiResult AppendTsTrack(bool aging, const MsprofCompactInfo *data);
    msptiResult AppendNodeLunch(bool aging, const MsprofApi *data);
    msptiResult AppendCommunication(bool aging, const MsprofApi *data);

private:
    msptiResult Run();
    msptiResult Analysis(const CannThreadCachePtr &cache);
    CannThreadCachePtr GetOrCreateCache(uint64_t threadId);
    bool MountCompactInfo(ApiEvent &apiEvent, const CannThreadCachePtr &cache);
    bool IsCommunicationNode(const MsprofApi &nodeLaunchApi, const CannThreadCachePtr &cache);
    void MountCommunicationNode(ApiEvent &apiEvent, const CannThreadCachePtr &cache);

private:
    std::mutex threadCachesMutex_;
    std::unordered_map<std::uint64_t, CannThreadCachePtr> threadCaches_{};
    std::mutex targetThreadMutex_;
    std::vector<uint64_t> targetThreadId_{};
    std::condition_variable cv;
    std::atomic<bool> threadRun_{ false };
    std::thread t_;
};

CannTrackCache::CannTrackCacheImpl::~CannTrackCacheImpl()
{
    StopTask();
}

msptiResult CannTrackCache::CannTrackCacheImpl::StartTask()
{
    if (!t_.joinable()) {
        threadRun_.store(true);
        t_ = std::thread(std::bind(&CannTrackCache::CannTrackCacheImpl::Run, this));
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::CannTrackCacheImpl::StopTask()
{
    threadRun_.store(false);
    cv.notify_one();
    if (t_.joinable()) {
        t_.join();
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::CannTrackCacheImpl::Run()
{
    pthread_setname_np(pthread_self(), "CannTrackCache");
    while (threadRun_) {
        std::vector<uint64_t> threads;
        {
            std::unique_lock<std::mutex> lock(targetThreadMutex_);
            cv.wait(lock, [this] { return !targetThreadId_.empty() || !threadRun_.load(); });
            threads = std::move(targetThreadId_);
            targetThreadId_.clear();
        }
        if (!threadRun_.load()) {
            break;
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

msptiResult CannTrackCache::CannTrackCacheImpl::AppendTsTrack(bool agingFlag, const MsprofCompactInfo *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto cache = GetOrCreateCache(data->threadId);
    if (cache == nullptr) {
        MSPTI_LOGE("copy MsprofCompactInfo data failed");
        return MSPTI_ERROR_INNER;
    }
    std::unique_ptr<MsprofCompactInfo> copy;
    Mspti::Common::MsptiMakeUniquePtr(copy);
    if (UNLIKELY(copy == nullptr)) {
        MSPTI_LOGE("copy MsprofCompactInfo data failed");
        return MSPTI_ERROR_INNER;
    }
    errno_t res = memcpy_s(copy.get(), sizeof(MsprofCompactInfo), data, sizeof(MsprofCompactInfo));
    if (res != EOK) {
        MSPTI_LOGE("memcpy MsprofCompactInfo failed! threadId is %d", data->threadId);
        return MSPTI_ERROR_INNER;
    }
    std::shared_ptr<TagAging<MsprofCompactInfo>> agingCopy;
    Mspti::Common::MsptiMakeSharedPtr(agingCopy, agingFlag, std::move(copy));
    if (UNLIKELY(agingCopy == nullptr)) {
        MSPTI_LOGE("copy TagAging MsprofCompactInfo data failed");
        return MSPTI_ERROR_INNER;
    }
    std::lock_guard<std::mutex> lk(cache->taskMutex);
    cache->taskQueue.emplace(data->timeStamp, agingCopy);
    return MSPTI_SUCCESS;
}

// Queue的push pop的多线程放最后考虑
msptiResult CannTrackCache::CannTrackCacheImpl::AppendNodeLunch(bool agingFlag, const MsprofApi *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto cache = GetOrCreateCache(data->threadId);
    if (cache == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    {
        std::unique_ptr<MsprofApi> copy;
        Mspti::Common::MsptiMakeUniquePtr(copy);
        if (UNLIKELY(copy == nullptr)) {
            MSPTI_LOGE("copy NodeLunch MsprofApi data failed");
            return MSPTI_ERROR_INNER;
        }
        errno_t res = memcpy_s(copy.get(), sizeof(MsprofApi), data, sizeof(MsprofApi));
        if (res != EOK) {
            MSPTI_LOGE("memcpy nodeLaunch MsprofApi failed! threadId is %d", data->threadId);
            return MSPTI_ERROR_INNER;
        }
        std::shared_ptr<TagAging<MsprofApi>> agingCopy;
        Mspti::Common::MsptiMakeSharedPtr(agingCopy, agingFlag, std::move(copy));
        if (UNLIKELY(agingCopy == nullptr)) {
            MSPTI_LOGE("copy TagAging NodeLunch data failed");
            return MSPTI_ERROR_INNER;
        }
        cache->nodeLaunchQueue.Push(agingCopy);
    }
    {
        std::lock_guard<std::mutex> lk(targetThreadMutex_);
        targetThreadId_.push_back(data->threadId);
    }
    cv.notify_one(); // 通知消费者有新数据
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::CannTrackCacheImpl::AppendCommunication(bool agingFlag, const MsprofApi *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto cache = GetOrCreateCache(data->threadId);
    if (cache == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    std::unique_ptr<MsprofApi> copy;
    Mspti::Common::MsptiMakeUniquePtr(copy);
    if (UNLIKELY(copy == nullptr)) {
        MSPTI_LOGE("copy Communication MsprofApi data failed");
        return MSPTI_ERROR_INNER;
    }
    errno_t res = memcpy_s(copy.get(), sizeof(MsprofApi), data, sizeof(MsprofApi));
    if (res != EOK) {
        MSPTI_LOGE("memcpy nodeLaunch MsprofApi failed! threadId is %d", data->threadId);
        return MSPTI_ERROR_INNER;
    }
    std::shared_ptr<TagAging<MsprofApi>> agingCopy;
    Mspti::Common::MsptiMakeSharedPtr(agingCopy, agingFlag, std::move(copy));
    if (UNLIKELY(agingCopy == nullptr)) {
        MSPTI_LOGE("copy TagAging Communication data failed");
        return MSPTI_ERROR_INNER;
    }
    cache->communicationQueue.Push(agingCopy);
    return MSPTI_SUCCESS;
}

bool CannTrackCache::CannTrackCacheImpl::IsCommunicationNode(const MsprofApi &nodeLaunchApi,
    const CannThreadCachePtr &cache)
{
    if (cache->communicationQueue.IsEmpty()) {
        return false;
    }
    std::shared_ptr<TagAging<MsprofApi>> communication;
    cache->communicationQueue.Peek(communication);
    return InApiRange(nodeLaunchApi, *communication->data);
}

msptiResult CannTrackCache::CannTrackCacheImpl::Analysis(const CannThreadCachePtr &cache)
{
    while (!cache->nodeLaunchQueue.IsEmpty()) {
        std::shared_ptr<TagAging<MsprofApi>> nodeLaunchApi;
        cache->nodeLaunchQueue.Pop(nodeLaunchApi);
        if (!nodeLaunchApi) {
            continue;
        }
        std::unique_ptr<ApiEvent> api2TaskInfo;
        Mspti::Common::MsptiMakeUniquePtr(api2TaskInfo);
        if (!UNLIKELY(api2TaskInfo)) {
            MSPTI_LOGE("fail to malloc api2TaskInfo");
            return MSPTI_ERROR_INNER;
        }
        api2TaskInfo->agingFlag = nodeLaunchApi->agingFlag;
        api2TaskInfo->api = *nodeLaunchApi->data;
        api2TaskInfo->threadId = nodeLaunchApi->data->threadId;
        api2TaskInfo->level = nodeLaunchApi->data->level;
        if (IsCommunicationNode(*nodeLaunchApi->data, cache)) {
            MountCommunicationNode(*api2TaskInfo, cache);
            CommunicationCalculator::GetInstance().AppendApi2TaskInfo(api2TaskInfo);
        } else {
            MountCompactInfo(*api2TaskInfo, cache);
        }
    }
    return MSPTI_SUCCESS;
}

void CannTrackCache::CannTrackCacheImpl::MountCommunicationNode(ApiEvent &apiEvent, const CannThreadCachePtr &cache)
{
    std::shared_ptr<TagAging<MsprofApi>> frontApi;
    while (!cache->communicationQueue.IsEmpty()) {
        cache->communicationQueue.Pop(frontApi);
        std::unique_ptr<ApiEvent> communicationTask;
        Mspti::Common::MsptiMakeUniquePtr(communicationTask);
        if (!UNLIKELY(communicationTask)) {
            MSPTI_LOGE("fail to malloc communicationTask");
            return;
        }
        communicationTask->threadId = apiEvent.threadId;
        communicationTask->api = *frontApi->data;
        communicationTask->level = communicationTask->api.level;
        if (MountCompactInfo(*communicationTask, cache)) {
            apiEvent.childs.emplace_back(std::move(communicationTask));
        }
    }
}

bool CannTrackCache::CannTrackCacheImpl::MountCompactInfo(ApiEvent &apiEvent, const CannThreadCachePtr &cache)
{
    std::lock_guard<std::mutex> lk(cache->taskMutex);
    if (cache->taskQueue.empty()) {
        return false;
    }
    auto startRuntimeTask = cache->taskQueue.lower_bound(apiEvent.api.beginTime);
    auto endRuntimeTask = cache->taskQueue.upper_bound(apiEvent.api.endTime);
    if (std::distance(startRuntimeTask, endRuntimeTask) == 0) {
        return false;
    }
    apiEvent.agingFlag = startRuntimeTask->second->agingFlag;
    apiEvent.compactInfo = *startRuntimeTask->second->data;
    cache->taskQueue.erase(startRuntimeTask, endRuntimeTask);
    return true;
}

CannThreadCachePtr CannTrackCache::CannTrackCacheImpl::GetOrCreateCache(uint64_t threadId)
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
    cache->threadId = threadId;
    threadCaches_[threadId] = cache;
    return cache;
}

CannTrackCache::CannTrackCache() : pImpl(std::make_unique<CannTrackCacheImpl>()) {}

CannTrackCache::~CannTrackCache() = default;

msptiResult CannTrackCache::AppendTsTrack(bool aging, const MsprofCompactInfo *data)
{
    return pImpl->AppendTsTrack(aging, data);
}

msptiResult CannTrackCache::AppendNodeLunch(bool aging, const MsprofApi *data)
{
    return pImpl->AppendNodeLunch(aging, data);
}

msptiResult CannTrackCache::AppendCommunication(bool aging, const MsprofApi *data)
{
    return pImpl->AppendCommunication(aging, data);
}

CannTrackCache &CannTrackCache::GetInstance()
{
    static CannTrackCache instance;
    return instance;
}

msptiResult CannTrackCache::StartTask()
{
    return pImpl->StartTask();
}

msptiResult CannTrackCache::StopTask()
{
    return pImpl->StopTask();
}
}
}