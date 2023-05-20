/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: analyze hwts data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#include "analyzer_hwts.h"
#include "data_struct.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "toolchain/prof_acl_api.h"
namespace Analysis {
namespace Dvvp {
namespace Analyze {
bool AnalyzerHwts::IsHwtsData(const std::string &fileName)
{
    // hwts data contains "hwts.data"
    if (fileName.find("hwts.data") != std::string::npos) {
        return true;
    }
    return false;
}

void AnalyzerHwts::Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        return;
    }
    totalBytes_ += static_cast<uint64_t>(message->chunksizeinbytes());
    ParseHwtsData(message->chunk().c_str(), message->chunksizeinbytes());
}

void AnalyzerHwts::ParseHwtsData(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainingLen = dataLen_ - offset;
        uint8_t rptType = GetRptType(dataPtr_ + offset, remainingLen);
        if (remainingLen < HWTS_DATA_SIZE) {
            // remaining is less then HWTS_DATA_SIZE, cache it to buffer
            MSPROF_LOGI("Hwts remains %u bytes unparsed, cache it", remainingLen);
            break;
        }
        if (rptType == HWTS_TASK_START_TYPE || rptType == HWTS_TASK_END_TYPE) {
            ParseTaskStartEndData(dataPtr_ + offset, dataLen_ - offset, rptType);
        }
        offset += HWTS_DATA_SIZE;
    }
    MSPROF_LOGI("Finish parsing hwts data, offset: %u, total len: %u, from buffered len: %u, "
                "op time collected %u, draft %u",
                offset, dataLen_, dataLen_ - len, opTimes_.size(), opTimeDrafts_.size());
    BufferRemainingData(offset);
}

uint8_t AnalyzerHwts::GetRptType(CONST_CHAR_PTR data, uint32_t len)
{
    if (len >= sizeof(uint8_t)) {
        auto firstByte = reinterpret_cast<const uint8_t *>(data);
        uint8_t rptType = (*firstByte) & 0x7;   // bit 0-3
        return rptType;
    } else {
        return HWTS_INVALID_TYPE;
    }
}

void AnalyzerHwts::CheckData(const struct OpTime &draftsOp, std::string key, uint8_t rptType, uint64_t sysTime)
{
    if (rptType == HWTS_TASK_START_TYPE) {
        if (draftsOp.start != 0) {
            MSPROF_LOGI("receive op repeats start-time. key:%s, LastStartTime:%llu, CurStartTime:%llu",
                        key.c_str(), draftsOp.start, sysTime);
            opRepeatCount_ += 1;
        }
        if (draftsOp.end != 0) {
            MSPROF_LOGI("receive op start-time later than end-time. key:%s, CurStartTime:%llu, LastEndTime:%llu",
                        key.c_str(), sysTime, draftsOp.end);
        }
    }
    if (rptType == HWTS_TASK_END_TYPE) {
        if (draftsOp.end != 0) {
            MSPROF_LOGI("receive op repeats end-time. key:%s, LastEndTime:%llu, CurEndTime:%llu",
                        key.c_str(), draftsOp.end, sysTime);
            opRepeatCount_ += 1;
        }
        if (draftsOp.start == 0) {
            MSPROF_LOGI("receive op end-time earlier than start-time. key:%s, CurEndTime:%llu",
                        key.c_str(), sysTime);
        }
    }
}

void AnalyzerHwts::ParseTaskStartEndData(CONST_CHAR_PTR data, uint32_t len, uint8_t rptType)
{
    if (len >= HWTS_DATA_SIZE) {
        auto hwtsData = reinterpret_cast<const HwtsProfileType01 *>(data);
        if (hwtsData == nullptr) {
            MSPROF_LOGE("Failed to call reinterpret_cast.");
            return;
        }
        std::string key = std::to_string(hwtsData->taskId) + KEY_SEPARATOR + std::to_string(hwtsData->streamId) +
            KEY_SEPARATOR + std::to_string(UINT32_MAX);
        auto iter = opTimeDrafts_.find(key);
        if (iter == opTimeDrafts_.end()) {
            OpTime opTime = {0, 0, 0, 0, 0, 0, ACL_SUBSCRIBE_OP};
            iter = opTimeDrafts_.insert(std::make_pair(key, opTime)).first;
        }
        uint64_t sysTime = static_cast<uint64_t>(hwtsData->syscnt / frequency_);  // ns
        switch (rptType) {
            case HWTS_TASK_START_TYPE:
                CheckData(iter->second, key, rptType, sysTime);
                iter->second.start = sysTime;
                break;
            case HWTS_TASK_END_TYPE:
                CheckData(iter->second, key, rptType, sysTime);
                iter->second.end = sysTime;
                break;
            default:
                MSPROF_LOGW("invalid rptType:%u, key:%s, cntRes0Type:0x%x, hex6bd3:0x%x",
                            rptType, key.c_str(), hwtsData->cntRes0Type, hwtsData->hex6bd3);
                break;
        }
        analyzedBytes_ += HWTS_DATA_SIZE;
        if (iter->second.start > 0 && iter->second.end > 0) {
            MSPROF_LOGD("Hwts op time collected, key %s, start %llu, end %llu",
                        key.c_str(), iter->second.start, iter->second.end);
            opTimes_.insert(std::make_pair(iter->first, iter->second));
            opTimeCount_++;
            opTimeDrafts_.erase(iter);
        }
    }
}

void AnalyzerHwts::PrintStats()
{
    MSPROF_EVENT("total_size_analyze, module: HWTS, analyzed %llu, total %llu, hwts time %u, merge %u",
                 analyzedBytes_, totalBytes_, totalHwtsTimes_, totalHwtsMerges_);
}

void AnalyzerHwts::HwtsParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        return;
    }
    totalBytes_ += message->chunksizeinbytes();
    ParseOptimizeHwtsData(message->chunk().c_str(), message->chunksizeinbytes());
}

void AnalyzerHwts::ParseOptimizeHwtsData(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainingLen = dataLen_ - offset;
        if (remainingLen < HWTS_DATA_SIZE) {
            MSPROF_LOGI("Hwts remains %u bytes unparsed, which is incomplete data", remainingLen);
            break;
        }
 
        uint8_t rptType = GetRptType(dataPtr_ + offset, remainingLen);
        if (rptType == HWTS_TASK_START_TYPE || rptType == HWTS_TASK_END_TYPE) {
            ParseTaskStartEndData(dataPtr_ + offset, dataLen_ - offset, rptType);
            ParseOptimizeStartEndData(dataPtr_ + offset, rptType);
            analyzedBytes_ += HWTS_DATA_SIZE;
            totalHwtsTimes_++;
        }
        offset += HWTS_DATA_SIZE;
    }
    BufferRemainingData(offset);
}

void AnalyzerHwts::ParseOptimizeStartEndData(CONST_CHAR_PTR data, uint8_t rptType)
{
    auto hwtsData = reinterpret_cast<const HwtsProfileType01 *>(data);
    std::string key = std::to_string(hwtsData->taskId) + KEY_SEPARATOR + std::to_string(hwtsData->streamId);
    auto iter = AnalyzerBase::tsOpInfo_.find(key);
    if (iter == AnalyzerBase::tsOpInfo_.end()) {
        RtOpInfo opInfo = {0, 0, 0, 0, true, 0, 0, ACL_SUBSCRIBE_OP};
        iter = AnalyzerBase::tsOpInfo_.insert(std::make_pair(key, opInfo)).first;
    }
 
    uint64_t sysTime = static_cast<uint64_t>(hwtsData->syscnt / frequency_);
    switch (rptType) {
        case HWTS_TASK_START_TYPE:
            iter->second.start = sysTime;
            break;
        case HWTS_TASK_END_TYPE:
            iter->second.end = sysTime;
            break;
        default:
            MSPROF_LOGW("invalid rptType:%u, key:%s, cntRes0Type:0x%x, hex6bd3:0x%x",
                        rptType, key.c_str(), hwtsData->cntRes0Type, hwtsData->hex6bd3);
            break;
    }
 
    if (AnalyzerBase::rtOpInfo_.find(key) != AnalyzerBase::rtOpInfo_.end()) {
        HandleDeviceData(key, iter->second, hwtsData->taskId, hwtsData->streamId, totalHwtsMerges_);
    }
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
