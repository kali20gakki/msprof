/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cann_track_cache.h
 * Description        : cann侧解析建树
 * Author             : msprof team
 * Creation Date      : 2025/05/14
 * *****************************************************************************
*/

#ifndef MSPTI_PARSER_CANN_TRACK_CACHE_H
#define MSPTI_PARSER_CANN_TRACK_CACHE_H

#include <thread>
#include <atomic>
#include "common/inject/profapi_inject.h"
#include "common/task.h"
#include "common/bound_queue.h"
#include "unordered_map"

namespace Mspti {
namespace Parser {
struct ApiEvent {
    uint64_t threadId;
    uint16_t level;
    MsprofApi api;
    MsprofCompactInfo compactInfo;
    std::vector<std::shared_ptr<ApiEvent>> childs;
};

class ProfTask {
public:
    explicit ProfTask() = default;
    virtual ~ProfTask() = default;
    virtual msptiResult StartTask() = 0;
    virtual msptiResult StopTask() = 0;
};

class NullProfTask : public ProfTask {
public:
    static NullProfTask& GetInstance()
    {
        static NullProfTask instance;
        return instance;
    }
    ~NullProfTask() override = default;
    msptiResult StartTask() override
    {
        return MSPTI_SUCCESS;
    }

    msptiResult StopTask() override
    {
        return MSPTI_SUCCESS;
    }

private:
    explicit NullProfTask() = default;
};

struct CannThreadCache {
    uint32_t threadId;
    Mspti::Common::MPSCQueue<std::shared_ptr<MsprofCompactInfo>, Mspti::Common::DEFAULT_CAPCAITY> taskQueue;
    Mspti::Common::MPSCQueue<std::shared_ptr<MsprofApi>, Mspti::Common::DEFAULT_CAPCAITY> nodeLaunchQueue;
    Mspti::Common::MPSCQueue<std::shared_ptr<MsprofApi>, Mspti::Common::DEFAULT_CAPCAITY> communicationQueue;
};

using CannThreadCachePtr = std::shared_ptr<CannThreadCache>;

class CannTrackCache final : public ProfTask {
public:
    msptiResult AppendTsTrack(const MsprofCompactInfo* data);

    msptiResult AppendNodeLunch(const MsprofApi* data);

    msptiResult AppendCommunication(const MsprofApi* data);

    ~CannTrackCache() override;

    static CannTrackCache& GetInstance()
    {
        static CannTrackCache instance;
        return instance;
    }

private:
    explicit CannTrackCache() = default;

    msptiResult Run();

    msptiResult StartTask() override;

    msptiResult StopTask() override;

    msptiResult Analysis(const CannThreadCachePtr& cache);

    CannThreadCachePtr GetOrCreateCache(uint64_t threadId);

    void MountCompactInfo(ApiEvent& apiEvent, const CannThreadCachePtr& cache);

    bool IsCommunicationNode(const MsprofApi &nodeLaunchApi, const CannThreadCachePtr& cache);

    void MountCommunicationNode(ApiEvent &apiEvent, const CannThreadCachePtr& cache);

private:
    std::mutex threadCachesMutex_;
    std::unordered_map<std::uint64_t, CannThreadCachePtr> threadCaches_;

    std::mutex targetThreadMutex_;
    std::vector<uint64_t> targetThreadId_;

    std::mutex cvMutex_;
    std::condition_variable cv;
    std::atomic<bool> threadRun_{false};
    std::thread t_;
};
}
}

#endif // MSPTI_PARSER_CANN_TRACK_CACHE_H
