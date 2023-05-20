/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: analyze ts data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#include "analyzer_ts.h"

#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "toolchain/prof_acl_api.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
bool AnalyzerTs::IsTsData(const std::string &fileName)
{
    // ts data contains "ts_track.data"
    if (fileName.find("ts_track.data") != std::string::npos) {
        return true;
    }
    return false;
}

void AnalyzerTs::Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        return;
    }

    totalBytes_ += static_cast<uint64_t>(message->chunksizeinbytes());
    ParseTsTrackData(message->chunk().c_str(), message->chunksizeinbytes());
}

void AnalyzerTs::ParseTsTrackData(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainingLen = dataLen_ - offset;
        if (remainingLen < sizeof(TsProfileDataHead)) {
            // remaining is less then TsProfileDataHead, cache it to buffer
            MSPROF_LOGI("Ts remains %u bytes unparsed, cache it", remainingLen);
            break;
        }
        auto tsHeader = reinterpret_cast<const TsProfileDataHead *>(dataPtr_ + offset);
        if (tsHeader == nullptr) {
            MSPROF_LOGE("Failed to call reinterpret_cast.");
            return;
        }
        if (tsHeader->bufSize == 0) {
            // invalid data, reset buffer
            MSPROF_LOGE("TsHeader buf size is 0, invalid data");
            BufferRemainingData(dataLen_);
            return;
        }
        if (remainingLen < tsHeader->bufSize) {
            // remaining is not enough for bufSize to parse, cache it to buffer
            MSPROF_LOGI("Ts remains %u bytes unparsed, bufSize %u, cache it", remainingLen, tsHeader->bufSize);
            break;
        }
        if (tsHeader->rptType == TS_TIMELINE_RPT_TYPE) {
            // data check ok, parse it
            ParseTsTimelineData(dataPtr_ + offset, remainingLen);
        } else if (tsHeader->rptType == TS_KEYPOINT_RPT_TYPE) {
            ParseTsKeypointData(dataPtr_ + offset, remainingLen);
        }
        offset += tsHeader->bufSize;
    }
    MSPROF_LOGI("Finish parsing tstrack data, offset: %u, total len: %u, from buffered len: %u, "
                "op time collected %u, draft %u",
                offset, dataLen_, dataLen_ - len, opTimes_.size(), opTimeDrafts_.size());
    BufferRemainingData(offset);
}

void AnalyzerTs::ParseTsTimelineData(CONST_CHAR_PTR data, uint32_t len)
{
    if (len >= sizeof(TsProfileTimeline)) {
        auto tsData = reinterpret_cast<const TsProfileTimeline *>(data);
        if (tsData == nullptr) {
            MSPROF_LOGE("Failed to call reinterpret_cast.");
            return;
        }
        std::string key = std::to_string(tsData->taskId) + KEY_SEPARATOR + std::to_string(tsData->streamId) +
                          KEY_SEPARATOR + std::to_string(UINT32_MAX);
        std::string optKey = std::to_string(tsData->taskId) + KEY_SEPARATOR + std::to_string(tsData->streamId);
        auto iter = opTimeDrafts_.find(key);
        if (iter == opTimeDrafts_.end()) {
            OpTime opTime = {0, 0, 0, 0, 0, 0, ACL_SUBSCRIBE_OP};
            iter = opTimeDrafts_.insert(std::make_pair(key, opTime)).first;
        }
        switch (tsData->taskState) {
            case TS_TIMELINE_START_TASK_STATE:
                iter->second.start = static_cast<uint64_t>(tsData->timestamp / frequency_);
                break;
            case TS_TIMELINE_AICORE_START_TASK_STATE:
                iter->second.startAicore = static_cast<uint64_t>(tsData->timestamp / frequency_);
                break;
            case TS_TIMELINE_AICORE_END_TASK_STATE:
                iter->second.endAicore = static_cast<uint64_t>(tsData->timestamp / frequency_);
                break;
            case TS_TIMELINE_END_TASK_STATE:
                iter->second.end = static_cast<uint64_t>(tsData->timestamp / frequency_);
                break;
            default:
                MSPROF_LOGD("AnalyzerTs dropped timeline task state: %u", tsData->taskState);
        }
        if (iter->second.start > 0 &&
            iter->second.startAicore > 0 &&
            iter->second.endAicore > 0 &&
            iter->second.end > 0) {
            if (AnalyzerBase::rtOpInfo_.find(optKey) != AnalyzerBase::rtOpInfo_.end()) {
                RtOpInfo devOpInfo = {0, iter->second.start, iter->second.end, 0, true, iter->second.startAicore,
                    iter->second.endAicore, ACL_SUBSCRIBE_OP};
                HandleDeviceData(optKey, devOpInfo, static_cast<uint32_t>(tsData->taskId), tsData->streamId,
                    totalTsMerges_);
            }
            MSPROF_LOGD("Ts op time collected, key %s, start %llu, end %llu",
                        key.c_str(), iter->second.start, iter->second.end);
            opTimes_.insert(std::make_pair(iter->first, iter->second));
            opTimeCount_++;
            opTimeDrafts_.erase(iter);
        }
        analyzedBytes_ += sizeof(TsProfileTimeline);
    }
}

void AnalyzerTs::ParseTsKeypointData(CONST_CHAR_PTR data, uint32_t len)
{
    if (len < sizeof(TsProfileKeypoint)) {
        return;
    }

    auto tsData = reinterpret_cast<const TsProfileKeypoint *>(data);
    if (tsData == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return;
    }
    if (tsData->head.bufSize != sizeof(TsProfileKeypoint) || !tsData->timestamp) {
        MSPROF_LOGE("keypoint op error. bufSize %llu, struct_len %u, "
                    "indexId %llu, taskId %u, streamId %u, timestamp %llu",
                    tsData->head.bufSize, sizeof(TsProfileKeypoint),
                    tsData->indexId, tsData->taskId, tsData->streamId, tsData->timestamp);
        return;
    }

    if (tsData->tagId == TS_KEYPOINT_START_TASK_STATE) {
        if (!keypointOpInfo_.empty() && !keypointOpInfo_.back().endTime) {
            MSPROF_LOGE("last keypoint op's end time is 0. indexId %llu, taskId %u, streamId %u",
                        tsData->indexId, tsData->taskId, tsData->streamId);
            return;
        }
        KeypointOp opData = {0, 0, 0, 0, 0, 0, 0, 0};
        opData.startTime = static_cast<uint64_t>(tsData->timestamp / frequency_);
        opData.indexId = tsData->indexId;
        opData.modelId = tsData->modelId;
        opData.streamId = tsData->streamId;
        opData.taskId = tsData->taskId;
        opData.uploaded = false;
        keypointOpInfo_.push_back(opData);
    } else if (tsData->tagId == TS_KEYPOINT_END_TASK_STATE && !keypointOpInfo_.empty()) {
        KeypointOp &lastOp = keypointOpInfo_.back();
        uint64_t ts = static_cast<uint64_t>(tsData->timestamp / frequency_);
        if (lastOp.endTime || ts <= lastOp.startTime) {
            MSPROF_LOGE("keypoint op error. indexId %llu, taskId %u, streamId %u, "
                        "startTime %llu, endTime %llu, timestamp %llu",
                        tsData->indexId, tsData->taskId, tsData->streamId,
                        lastOp.startTime, lastOp.endTime, ts);
            return;
        } else {
            lastOp.endTime = ts;
            MSPROF_LOGD("keypoint op: startTime %llu, endTime %llu, indexId=%llu, endSyscnt %llu",
                        lastOp.startTime, lastOp.endTime, lastOp.indexId, tsData->timestamp);
        }
    } else {
        MSPROF_LOGE("keypoint tagId error. tagId %u, keypointOp %u", tsData->tagId, keypointOpInfo_.size());
        return;
    }
    analyzedBytes_ += sizeof(TsProfileKeypoint);
}

void AnalyzerTs::PrintStats()
{
    uint64_t times = 0;
    if (!keypointOpInfo_.empty()) {
        times = keypointOpInfo_.back().findSuccTimes;
    }
    MSPROF_EVENT("total_size_analyze, module: TS, analyzed %llu, total %llu, op time total %llu, "
                 "remain %u, draft %u. remain keypoint op %llu, last step find op %llu, merge time %u",
                 analyzedBytes_, totalBytes_, opTimeCount_, opTimes_.size(), opTimeDrafts_.size(),
                 keypointOpInfo_.size(), times, totalTsMerges_);
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
