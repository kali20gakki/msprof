/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: parse ffts data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-02-10
 */
#include "analyzer_ffts.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "toolchain/prof_acl_api.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
bool AnalyzerFfts::IsFftsData(const std::string &fileName) const
{
    // "ffts.data"
    if (fileName.find("stars_soc.data") != std::string::npos) {
        return true;
    }
    return false;
}

void AnalyzerFfts::PrintStats() const
{
    MSPROF_EVENT("total_size_analyze, module: FFTS, analyzed %llu, total %llu, ffts time total %llu, merge %u",
                 analyzedBytes_, totalBytes_, totalFftsTimes_, totalFftsMerges_);
}

void AnalyzerFfts::FftsParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (fileChunkReq == nullptr) {
        return;
    }
    totalBytes_ += fileChunkReq->chunkSize;
    ParseOptimizeFftsData(fileChunkReq->chunk.c_str(), fileChunkReq->chunkSize);
}

void AnalyzerFfts::ParseOptimizeFftsData(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t opNum = 0;
    uint32_t curLen = 0;
    uint32_t unknownOpNum = 0;
    while (dataPtr_ != nullptr && curLen < dataLen_) {
        uint32_t remainLen = dataLen_ - curLen;
        if (remainLen < FFTS_DATA_SIZE) {
            MSPROF_LOGW("Ffts remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }
        auto fftsLogOptHead = reinterpret_cast<const FftsLogHead *>(dataPtr_ + curLen);
        uint32_t logType = fftsLogOptHead->logType;
        if (logType == ACSQ_TASK_START_FUNC_TYPE || logType == ACSQ_TASK_END_FUNC_TYPE) {
            ParseOptimizeAcsqTaskData(fftsLogOptHead, logType);
        } else if (logType == FFTS_SUBTASK_THREAD_START_FUNC_TYPE || logType == FFTS_SUBTASK_THREAD_END_FUNC_TYPE) {
            ParseOptimizeSubTaskThreadData(fftsLogOptHead, logType);
        } else {
            MSPROF_LOGD("unknownOp ffts op. logType:%u", logType);
            unknownOpNum++;
        }
        opNum += 1;
        curLen += FFTS_DATA_SIZE;
        analyzedBytes_ += FFTS_DATA_SIZE;
        totalFftsTimes_++;
    }
    BufferRemainingData(curLen);
}

/*
 * @berif  : parse op info with acsq task info
 * @param  : None
 * @return : None
 */
void AnalyzerFfts::ParseOptimizeAcsqTaskData(const FftsLogHead *data, uint32_t logType)
{
    if (data == nullptr) {
        MSPROF_LOGE("null acsq task data.");
        return;
    }
    auto acsqLog = reinterpret_cast<const FftsAcsqLog *>(data);
    std::string key = std::to_string(acsqLog->taskId) + KEY_SEPARATOR + std::to_string(acsqLog->streamId);
    auto iter = AnalyzerBase::tsOpInfo_.find(key);
    if (iter == AnalyzerBase::tsOpInfo_.end()) {
        RtOpInfo opInfo = {0, 0, 0, 0, true, 0, 0, ACL_SUBSCRIBE_OP, UINT16_MAX};
        iter = AnalyzerBase::tsOpInfo_.insert(std::make_pair(key, opInfo)).first;
    }
    constexpr uint32_t offsetBit = 32;
    uint64_t sysTime = ((static_cast<uint64_t>(acsqLog->sysCountHigh) << offsetBit) | acsqLog->sysCountLow);
    if (logType == ACSQ_TASK_START_FUNC_TYPE) {
        iter->second.start = static_cast<uint64_t>(sysTime / frequency_);  // to ns
    } else {
        iter->second.end = static_cast<uint64_t>(sysTime / frequency_);
    }
    if (iter->second.start > 0 && iter->second.end > 0) {
        HandleDeviceData(key, iter->second, totalFftsMerges_);
    }
}

/*
 * @berif  : parse op info with sub task thead info
 * @param  : None
 * @return : None
 */
void AnalyzerFfts::ParseOptimizeSubTaskThreadData(const FftsLogHead *data, uint32_t logType)
{
    if (data == nullptr) {
        MSPROF_LOGE("null sub task thread data.");
        return;
    }
    auto cxtLog = reinterpret_cast<const FftsCxtLog *>(data);
    std::string key = std::to_string(cxtLog->taskId) + KEY_SEPARATOR + std::to_string(cxtLog->streamId);
    auto iter = AnalyzerBase::tsOpInfo_.find(key);
    if (iter == AnalyzerBase::tsOpInfo_.end()) {
        RtOpInfo opInfo = {0, 0, 0, 0, true, 0, 0, ACL_SUBSCRIBE_OP_THREAD, cxtLog->cxtId};
        iter = AnalyzerBase::tsOpInfo_.insert(std::make_pair(key, opInfo)).first;
    }
 
    constexpr uint32_t offsetBit = 32;
    uint64_t sysTime = ((static_cast<uint64_t>(cxtLog->sysCountHigh) << offsetBit) | cxtLog->sysCountLow);
    if (logType == FFTS_SUBTASK_THREAD_START_FUNC_TYPE) {
        iter->second.start = static_cast<uint64_t>(sysTime / frequency_);  // to ns
    } else {
        iter->second.end = static_cast<uint64_t>(sysTime / frequency_);
    }
    analyzedBytes_ += FFTS_DATA_SIZE;
    if (iter->second.start > 0 && iter->second.end > 0) {
        HandleDeviceData(key, iter->second, totalFftsMerges_);
    }
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
