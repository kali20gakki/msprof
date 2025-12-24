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

#include "csrc/activity/ascend/parser/cann_track_cache.h"

#include <functional>
#include <unordered_map>
#include <algorithm>
#include "securec.h"
#include "csrc/activity/ascend/parser/cann_hash_cache.h"
#include "csrc/activity/ascend/parser/communication_calculator.h"
#include "csrc/common/plog_manager.h"
#include "csrc/common/utils.h"
#include "csrc/common/mpsc_queue.h"
#include "csrc/common/context_manager.h"
#include "csrc/activity/activity_manager.h"

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

template <typename T> struct Tag {
    T data{};
    bool agingFlag = true;
    uint64_t correlationId = 0;
    uint64_t parentNodeLaunchId = 0;
    Tag() = default;
    explicit Tag(const T& value) : data(value), agingFlag(true) {}
    explicit Tag(T &&value) : data(std::move(value)), agingFlag(true) {}
};

struct CannThreadCache {
    std::atomic<uint32_t> nodeLaunchId{0};
    uint32_t threadId{};
    std::mutex taskMutex;
    Tag<MsprofCompactInfo> lastTask;
    ApiEvent lastCommunication;
    std::vector<Tag<MsprofCompactInfo>>  taskQueue;
    explicit CannThreadCache(uint32_t tid): threadId(tid), lastTask(), lastCommunication() {};
    uint64_t GetNodeLaunchId()
    {
        uint64_t ans = (static_cast<uint64_t>(threadId) << 32) | nodeLaunchId.load();
        return ans;
    }
    uint64_t UpdateAndGetNodeLaunchId()
    {
        auto launchId = nodeLaunchId.fetch_add(1);
        uint64_t ans = (static_cast<uint64_t>(threadId) << 32) | launchId;
        return ans;
    }
};

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
    CannThreadCache& GetOrCreateCache(uint32_t threadId);

private:
    std::mutex threadCachesMutex_;
    std::unordered_map<std::uint64_t, CannThreadCache> threadCaches_{};
    std::vector<uint64_t> targetThreadId_{};
    std::thread t_;
};

CannTrackCache::CannTrackCacheImpl::~CannTrackCacheImpl()
{
    StopTask();
}

msptiResult CannTrackCache::CannTrackCacheImpl::StartTask()
{
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::CannTrackCacheImpl::StopTask()
{
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::CannTrackCacheImpl::AppendTsTrack(bool agingFlag, const MsprofCompactInfo *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto& cache = GetOrCreateCache(data->threadId);
    cache.lastTask.agingFlag = agingFlag;
    cache.lastTask.data = *data;
    return MSPTI_SUCCESS;
}

// Queue的push pop的多线程放最后考虑
msptiResult CannTrackCache::CannTrackCacheImpl::AppendNodeLunch(bool agingFlag, const MsprofApi *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto& cache = GetOrCreateCache(data->threadId);
    auto lastCommunicationTask = cache.lastCommunication.api;
    if (!InApiRange(*data, lastCommunicationTask)) {
        return MSPTI_SUCCESS;
    }

    ApiEvent nodeLaunchEvent;
    nodeLaunchEvent.correlationId = Mspti::Common::ContextManager::GetInstance()->UpdateAndReportCorrelationId(data->threadId);
    nodeLaunchEvent.parentEventId = 0;
    nodeLaunchEvent.eventId = cache.UpdateAndGetNodeLaunchId();
    nodeLaunchEvent.agingFlag = agingFlag;
    nodeLaunchEvent.api = *data;
    nodeLaunchEvent.children.emplace_back(cache.lastCommunication);
    CommunicationCalculator::GetInstance().AppendApi2TaskInfo(nodeLaunchEvent);
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::CannTrackCacheImpl::AppendCommunication(bool agingFlag, const MsprofApi *data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    auto& cache = GetOrCreateCache(data->threadId);
    if (!InApiRange(*data, cache.lastTask.data)) {
    return MSPTI_SUCCESS;
}

    ApiEvent communicationTask;
    communicationTask.parentEventId = cache.GetNodeLaunchId();
    communicationTask.threadId = data->threadId;
    communicationTask.api = *data;
    communicationTask.level = data->level;
    communicationTask.agingFlag = cache.lastTask.agingFlag;
    communicationTask.compactInfo = cache.lastTask.data;
    CommunicationCalculator::GetInstance().AppendCommunicationTask(communicationTask);
    cache.lastCommunication = communicationTask;
    return MSPTI_SUCCESS;
}

CannThreadCache& CannTrackCache::CannTrackCacheImpl::GetOrCreateCache(uint32_t threadId)
{
    static thread_local CannThreadCache cache(threadId);
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