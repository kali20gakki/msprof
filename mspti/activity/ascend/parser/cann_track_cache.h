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
#include <vector>
#include <memory>
#include "common/inject/profapi_inject.h"

namespace Mspti {
namespace Parser {
struct ApiEvent {
    bool agingFlag = true;
    uint64_t threadId;
    uint16_t level;
    MsprofApi api;
    MsprofCompactInfo compactInfo;
    std::vector<std::unique_ptr<ApiEvent>> childs;
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

class CannTrackCache final : public ProfTask {
public:
    static CannTrackCache& GetInstance();

    msptiResult AppendTsTrack(bool aging, const MsprofCompactInfo* data);

    msptiResult AppendNodeLunch(bool aging, const MsprofApi* data);

    msptiResult AppendCommunication(bool aging, const MsprofApi* data);

    msptiResult StartTask() override;

    msptiResult StopTask() override;
private:
    CannTrackCache();
    ~CannTrackCache() override;
    class CannTrackCacheImpl;
    std::unique_ptr<CannTrackCacheImpl> pImpl;
};
}
}

#endif // MSPTI_PARSER_CANN_TRACK_CACHE_H
