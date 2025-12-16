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

#ifndef MSPTI_PARSER_CANN_TRACK_CACHE_H
#define MSPTI_PARSER_CANN_TRACK_CACHE_H

#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "csrc/common/inject/profapi_inject.h"

namespace Mspti {
namespace Parser {
struct ApiEvent {
    uint16_t level;
    bool agingFlag = true;
    uint64_t threadId;
    uint64_t correlationId;
    MsprofApi api;
    MsprofCompactInfo compactInfo;
    std::vector<std::unique_ptr<ApiEvent>> children;
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
