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
 * -------------------------------------------------------------------------*/
#include "analyzer_ffts.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "toolchain/prof_acl_api.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
constexpr uint16_t TS_MASK_BIT0_BIT1   = 0x3U;    // 0b11
constexpr uint16_t TS_MASK_BIT0_BIT11  = 0x0FFFU; // take significant bit0~11
constexpr uint16_t TS_MASK_BIT12       = 0x1000U; // bit12 indicate whether taskId is update
constexpr uint16_t TS_MASK_BIT13_BIT15 = 0xE000U; // take significant bit13~15
constexpr uint16_t TS_MASK_BIT12_BIT15 = 0xF000U; // take significant bit12~15
constexpr uint16_t TS_UPDATE_FOR_ALL   = 0b10;
constexpr uint16_t TS_UPDATE_FLAG_BIT  = 12U;

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
 * @brief  : parse op info with acsq task info
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
    uint16_t taskId = acsqLog->taskId;
    uint16_t streamId = acsqLog->streamId;
    StarsRollBackStreamTaskId(&streamId, &taskId);
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
 * @brief  : parse op info with sub task thead info
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
    uint16_t taskId = cxtLog->taskId;
    uint16_t streamId = cxtLog->streamId;
    StarsRollBackStreamTaskId(&streamId, &taskId);
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

void AnalyzerFfts::StarsRollBackStreamTaskId(uint16_t *streamId, uint16_t *taskId) const
{
    // bit12 == 0 && bit13 == 1: streamId = (bit0~11)taskId, taskId = (bit0~11)streamId | (bit12~15)taskId
    if ((((*streamId) >> TS_UPDATE_FLAG_BIT) & TS_MASK_BIT0_BIT1) == TS_UPDATE_FOR_ALL) {
        uint16_t tmpTaskId = (*taskId);
        *taskId = ((*streamId) & TS_MASK_BIT0_BIT11) | ((*taskId) & TS_MASK_BIT12_BIT15);
        *streamId = tmpTaskId & TS_MASK_BIT0_BIT11;
        return;
    }
    // bit12 == 1: streamId = (bit0~11)streamId, taskId = (bit0~11)taskId | bit12 | (bit13~15)streamId
    if (((*streamId) & TS_MASK_BIT12) != 0U) {
        *taskId = (((*taskId) & (TS_MASK_BIT0_BIT11 | TS_MASK_BIT12)) | ((*streamId) & TS_MASK_BIT13_BIT15));
        *streamId &= TS_MASK_BIT0_BIT11;
    }
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
