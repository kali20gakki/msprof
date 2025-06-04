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
inline bool InApiRange(const MsprofApi &data, const MsprofCompactInfo& runtimeTrack)
{
    return runtimeTrack.timeStamp <= data.endTime && runtimeTrack.timeStamp >= data.beginTime;
}

inline bool InApiRange(const MsprofApi &data1, const MsprofApi& data2)
{
    return data1.beginTime <= data2.beginTime && data1.endTime >= data2.endTime;
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
        cv.wait(lock, [this] {
            return !targetThreadId_.empty() || !threadRun_.load();
        });
        if (!threadRun_.load()) {
            break;
        }

        std::vector<uint64_t> threads(0);
        {
            std::lock_guard<std::mutex> lk(targetThreadMutex_);
            threads = std::move(targetThreadId_);
            targetThreadId_.clear();
        }

        if (threads.empty()) {
            continue;
        }
        for (auto threadId : threads) {
            std::queue<std::shared_ptr<MsprofApi>> nodeLaunchs;
            {
                std::unique_lock<std::mutex> lock(nodeLaunchMutex_);
                nodeLaunchs = std::move(nodeLaunchQueue_[threadId]);
            }
            while (!nodeLaunchs.empty()) {
                Analysis(*nodeLaunchs.front());
                nodeLaunchs.pop();
            }
        }
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::AppendTsTrack(const MsprofCompactInfo* data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    std::unique_lock<std::mutex> lock(taskMutex_);
    MsprofCompactInfo* copy = Mspti::Common::ReinterpretConvert<MsprofCompactInfo*>(malloc(sizeof(MsprofCompactInfo)));
    if (copy == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    memcpy_s(copy, sizeof(MsprofCompactInfo), data, sizeof(MsprofCompactInfo));
    taskQueue_[data->threadId].emplace_back(copy);
    return MSPTI_SUCCESS;
}

// Queue的push pop的多线程放最后考虑
msptiResult CannTrackCache::AppendNodeLunch(const MsprofApi* data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    {
        std::lock_guard<std::mutex> lk(nodeLaunchMutex_);
        MsprofApi* copy = Mspti::Common::ReinterpretConvert<MsprofApi*>(malloc(sizeof(MsprofApi)));
        if (copy == nullptr) {
            return MSPTI_ERROR_INNER;
        }
        memcpy_s(copy, sizeof(MsprofApi), data, sizeof(MsprofApi));
        nodeLaunchQueue_[data->threadId].emplace(copy);
    }

    {
        std::lock_guard<std::mutex> lk(targetThreadMutex_);
        targetThreadId_.push_back(data->threadId);
    }

    {
        std::lock_guard<std::mutex> lock(cvMutex_);
        cv.notify_one();  // 通知消费者有新数据
    }
    return MSPTI_SUCCESS;
}

msptiResult CannTrackCache::AppendCommunication(const MsprofApi* data)
{
    if (!Activity::ActivityManager::GetInstance()->IsActivityKindEnable(MSPTI_ACTIVITY_KIND_COMMUNICATION)) {
        return MSPTI_SUCCESS;
    }
    std::lock_guard<std::mutex> lk(communicationMutex_);
    MsprofApi* copy = Mspti::Common::ReinterpretConvert<MsprofApi*>(malloc(sizeof(MsprofApi)));
    if (copy == nullptr) {
        return MSPTI_ERROR_INNER;
    }
    memcpy_s(copy, sizeof(MsprofApi), data, sizeof(MsprofApi));
    communicationQueue_[data->threadId].emplace(copy);
    return MSPTI_SUCCESS;
}

bool CannTrackCache::IsCommunicationNode(const MsprofApi &nodeLaunchApi)
{
    std::lock_guard<std::mutex> lk(communicationMutex_);
    if (!communicationQueue_.count(nodeLaunchApi.threadId) || communicationQueue_[nodeLaunchApi.threadId].empty()) {
        return false;
    }
    return InApiRange(nodeLaunchApi, *communicationQueue_[nodeLaunchApi.threadId].front());
}

msptiResult CannTrackCache::Analysis(const MsprofApi &nodeLaunchApi)
{
    std::shared_ptr<ApiEvent> api2TaskInfo;
    Mspti::Common::MsptiMakeSharedPtr(api2TaskInfo);
    if (!UNLIKELY(api2TaskInfo)) {
        MSPTI_LOGE("fail to malloc api2TaskInfo");
        return MSPTI_ERROR_INNER;
    }
    api2TaskInfo->api = nodeLaunchApi;
    api2TaskInfo->threadId = nodeLaunchApi.threadId;
    api2TaskInfo->level = nodeLaunchApi.level;
    if (IsCommunicationNode(nodeLaunchApi)) {
        MountCommunicationNode(*api2TaskInfo);
        CommunicationCalculator::GetInstance().AppendApi2TaskInfo(api2TaskInfo);
    } else {
        MountCompactInfo(*api2TaskInfo);
    }
    return MSPTI_SUCCESS;
}

void CannTrackCache::MountCommunicationNode(ApiEvent& apiEvent)
{
    std::lock_guard<std::mutex> lk(communicationMutex_);
    while (!communicationQueue_[apiEvent.threadId].empty()
           && InApiRange(apiEvent.api, *communicationQueue_[apiEvent.threadId].front())) {
        std::shared_ptr<ApiEvent> communicationTask;
        Mspti::Common::MsptiMakeSharedPtr(communicationTask);
        if (!UNLIKELY(communicationTask)) {
            MSPTI_LOGE("fail to malloc communicationTask");
            return;
        }
        communicationTask->threadId = apiEvent.threadId;
        communicationTask->api = *communicationQueue_[apiEvent.threadId].front();
        communicationTask->level = communicationTask->api.level;
        MountCompactInfo(*communicationTask);

        apiEvent.childs.emplace_back(communicationTask);
        communicationQueue_[apiEvent.threadId].pop();
    }
}

void CannTrackCache::MountCompactInfo(ApiEvent& apiEvent)
{
    auto threadId = apiEvent.threadId;
    std::lock_guard<std::mutex> lk(taskMutex_);
    auto it = taskQueue_.find(threadId);
    if (it == taskQueue_.end() || it->second.empty()) {
        return;
    }

    std::deque<std::shared_ptr<MsprofCompactInfo>> beforeQueue;
    auto& queue = it->second;
    while (!queue.empty() && queue.front()->timeStamp < apiEvent.api.beginTime) {
        beforeQueue.push_back(queue.front());
        queue.pop_front();
    }

    while (!queue.empty()) {
        auto curTask = queue.front();
        if (!InApiRange(apiEvent.api, *curTask)) {
            break;
        }
        apiEvent.compactInfo = *curTask;
        queue.pop_front();
    }

    if (!beforeQueue.empty()) {
        queue.insert(queue.end(), beforeQueue.begin(), beforeQueue.end());
    }
}

}
}