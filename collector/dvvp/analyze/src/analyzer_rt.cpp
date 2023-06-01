/**
 * @file analyzer_rt.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include "analyzer_rt.h"
#include "acl_prof.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "prof_common.h"
#include "prof_api.h"
 
namespace Analysis {
namespace Dvvp {
namespace Analyze {
static const uint32_t RT_COMPACT_INFO_SIZE = sizeof(MsprofCompactInfo);
 
bool AnalyzerRt::IsRtCompactData(const std::string &tag) const
{
    if (tag.find("task_track") != std::string::npos) {
        return true;
    }
 
    return false;
}

/*
 * @berif  : parse runtime task_track data
 * @param  : None
 * @return : None
 */
void AnalyzerRt::RtCompactParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        return;
    }
 
    totalBytes_ += message->chunksizeinbytes();
    if (message->filename().find("unaging") != std::string::npos) {
        ParseRuntimeTrackData(message->chunk().c_str(), message->chunksizeinbytes(), false);
        return;
    }
}

void AnalyzerRt::ParseRuntimeTrackData(CONST_CHAR_PTR data, uint32_t len, bool ageFlag)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainLen = dataLen_ - offset;
        if (remainLen < RT_COMPACT_INFO_SIZE) {
            MSPROF_LOGW("Runtime track remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }
 
        auto rtData = reinterpret_cast<const MsprofCompactInfo *>(dataPtr_ + offset);
        MSPROF_LOGD("ParseRuntimeTrackData level: %hu.", rtData->level);
        if (rtData->level == MSPROF_REPORT_RUNTIME_LEVEL) {
            HandleRuntimeTrackData(dataPtr_ + offset, ageFlag);
            totalRtTimes_++;
        }
 
        offset += RT_COMPACT_INFO_SIZE;
        analyzedBytes_ += RT_COMPACT_INFO_SIZE;
    }
    BufferRemainingData(offset);
}
 
void AnalyzerRt::HandleRuntimeTrackData(CONST_CHAR_PTR data, bool ageFlag)
{
    auto compactData = reinterpret_cast<const MsprofCompactInfo *>(data);
    auto rtData = compactData->data.runtimeTrack;
    std::string key = std::to_string(rtData.taskId) + KEY_SEPARATOR + std::to_string(rtData.streamId);
 
    auto hostIter = AnalyzerBase::rtOpInfo_.find(key);
    if (hostIter != AnalyzerBase::rtOpInfo_.end()) {
        EraseRtMapByStreamId(rtData.streamId, AnalyzerBase::rtOpInfo_);
        MSPROF_LOGD("Delete repeat runtime track data with same key. taskId: %u, streamId: %u.",
            rtData.taskId, rtData.streamId);
    }
 
    RtOpInfo opInfo = {compactData->timeStamp, 0, 0, compactData->threadId, ageFlag, 0, 0,
        ACL_SUBSCRIBE_OP, UINT16_MAX};
    AnalyzerBase::rtOpInfo_[key] = opInfo;
    MSPROF_LOGD("host aging data not found, insert runtime track data in rtOpInfo map"
        ", key: %s, timeStamp: %llu, age: %d", key.c_str(), compactData->timeStamp, ageFlag);
}
 
void AnalyzerRt::PrintStats() const
{
    MSPROF_EVENT("total_size_analyze, module: RT, analyzed %llu, total %llu, rt time %u, merge %u",
        analyzedBytes_, totalBytes_, totalRtTimes_, totalRtMerges_);
}
 
}
}
}