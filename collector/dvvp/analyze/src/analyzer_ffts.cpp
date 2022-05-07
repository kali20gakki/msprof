/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: parse ffts data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-02-10
 */
#include "analyzer_ffts.h"
#include "acl_prof.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "proto/profiler.pb.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
bool AnalyzerFfts::IsFftsData(const std::string &fileName)
{
    // "ffts.data"
    if (fileName.find("stars_soc.data") != std::string::npos) {
        return true;
    }
    return false;
}

void AnalyzerFfts::Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        return;
    }
    totalBytes_ += message->chunksizeinbytes();
    ParseData(message->chunk().c_str(), message->chunksizeinbytes());
}

void AnalyzerFfts::ParseData(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t parsedOpNum = 0;
    uint32_t parsedLen = 0;
    uint32_t unknownOpNum = 0;
    while (dataPtr_ != nullptr && parsedLen < dataLen_) {
        uint32_t remainLen = dataLen_ - parsedLen;
        if (remainLen < FFTS_DATA_SIZE) {
            // remaining is less then FFTS_DATA_SIZE, cache it to buffer
            MSPROF_LOGW("Ffts remains %u bytes unparsed, cache it", remainLen);
            break;
        }
        auto fftsLogHead = reinterpret_cast<const FftsLogHead *>(dataPtr_ + parsedLen);
        uint32_t logType = fftsLogHead->logType;
        if (logType == ACSQ_TASK_START_FUNC_TYPE || logType == ACSQ_TASK_END_FUNC_TYPE) {
            ParseAcsqTaskData(fftsLogHead, logType);
        } else if (logType == FFTS_SUBTASK_THREAD_START_FUNC_TYPE || logType == FFTS_SUBTASK_THREAD_END_FUNC_TYPE) {
            ParseSubTaskThreadData(fftsLogHead, logType);
        } else {
            MSPROF_LOGD("unknownOp ffts op. logType:%u", logType);
            unknownOpNum++;
        }
        parsedOpNum += 1;
        parsedLen += FFTS_DATA_SIZE;
    }
    MSPROF_LOGI("Finish parsing ffts data, BuffLen:%u NewDataLen:%u parsedLen:%u TotalOpNum:%u ParsedOp:%u, "
                "unknownOpNum:%u", dataLen_, len, parsedLen, opTimes_.size(), parsedOpNum, unknownOpNum);
    BufferRemainingData(parsedLen);
}

void AnalyzerFfts::ParseAcsqTaskData(const FftsLogHead *data, uint32_t logType)
{
    auto acsqLog = reinterpret_cast<const FftsAcsqLog *>(data);
    std::string key = std::to_string(acsqLog->taskId) + KEY_SEPARATOR +
        std::to_string(acsqLog->streamId) + KEY_SEPARATOR + std::to_string(UINT32_MAX);
    auto iter = opDrafts_.find(key);
    if (iter == opDrafts_.end()) {
        OpTime opTime = {0, 0, 0, 0, 0, 0, ACL_SUBSCRIBE_OP};  // default flag is OP not SUBGRAPH
        iter = opDrafts_.insert(std::make_pair(key, opTime)).first;
    }
    constexpr uint32_t offsetBit = 32;
    uint64_t sysTime = ((static_cast<uint64_t>(acsqLog->sysCountHigh) << offsetBit) | acsqLog->sysCountLow);
    if (logType == ACSQ_TASK_START_FUNC_TYPE) {
        iter->second.start = sysTime / frequency_;  // to ns
    } else {
        iter->second.end = sysTime / frequency_;
    }
    analyzedBytes_ += FFTS_DATA_SIZE;
    if (iter->second.start > 0 && iter->second.end > 0) {
        MSPROF_LOGD("Ffts acsq task op time collected, key %s, start %llu, end %llu",
                    key.c_str(), iter->second.start, iter->second.end);
        opTimes_.insert(std::make_pair(iter->first, iter->second));
        opTimeCount_++;
        opDrafts_.erase(iter);
    }
}

void AnalyzerFfts::ParseSubTaskThreadData(const FftsLogHead *data, uint32_t logType)
{
    auto cxtLog = reinterpret_cast<const FftsCxtLog *>(data);
    std::string key = std::to_string(cxtLog->taskId) + KEY_SEPARATOR +
        std::to_string(cxtLog->streamId) + KEY_SEPARATOR + std::to_string(cxtLog->cxtId);
    auto iter = opDrafts_.find(key);
    if (iter == opDrafts_.end()) {
        OpTime opTime = {0, 0, 0, 0, 0, 0, ACL_SUBSCRIBE_OP_THREAD};
        iter = opDrafts_.insert(std::make_pair(key, opTime)).first;
    }
    constexpr uint32_t offsetBit = 32;
    uint64_t sysTime = ((static_cast<uint64_t>(cxtLog->sysCountHigh) << offsetBit) | cxtLog->sysCountLow);
    if (logType == FFTS_SUBTASK_THREAD_START_FUNC_TYPE) {
        iter->second.start = sysTime / frequency_; // to ns
        iter->second.threadId = cxtLog->threadId;
    } else {
        iter->second.end = sysTime / frequency_;
        if (iter->second.threadId != cxtLog->threadId) {
            MSPROF_LOGE("Ffts subtask op thread threadId error, %s %llu %llu",
                        key.c_str(), iter->second.threadId, cxtLog->threadId);
        }
    }
    analyzedBytes_ += FFTS_DATA_SIZE;

    if (iter->second.start > 0 && iter->second.end > 0) {
        MSPROF_LOGD("Ffts subtask op thread time collected, key %s, start %llu, end %llu",
                    key.c_str(), iter->second.start, iter->second.end);
        opTimes_.insert(std::make_pair(iter->first, iter->second));
        opTimeCount_++;
        opDrafts_.erase(iter);
    }
}

void AnalyzerFfts::PrintStats()
{
    MSPROF_EVENT("total_size_analyze, module: FFTS, analyzed %llu, total %llu, "
                 "op time total %llu, remain %u, draft %u, op repeat cnt %llu",
                 analyzedBytes_, totalBytes_, opTimeCount_, opTimes_.size(),
                 opDrafts_.size(), opRepeatCount_);
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
