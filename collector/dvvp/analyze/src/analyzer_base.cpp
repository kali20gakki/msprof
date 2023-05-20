/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: buffer data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#include "analyzer_base.h"
#include "config/config_manager.h"
#include "transport/hash_data.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;
std::map<std::string, RtOpInfo> AnalyzerBase::rtOpInfo_;
std::map<std::string, RtOpInfo> AnalyzerBase::tsOpInfo_;
std::multimap<std::string, GeOpFlagInfo> AnalyzerBase::geNodeInfo_;
std::multimap<std::string, GeOpFlagInfo> AnalyzerBase::geApiInfo_;
std::multimap<std::string, GeOpFlagInfo> AnalyzerBase::geModelInfo_;
std::multimap<std::string, GeOpFlagInfo> AnalyzerBase::geOpInfo_;
std::map<uint32_t, uint32_t> AnalyzerBase::graphIdMap_;
std::vector<ProfOpDesc> AnalyzerBase::opDescInfos_;
std::mutex AnalyzerBase::opDescInfoMtx_;
std::mutex AnalyzerBase::graphIdMtx_;

void AnalyzerBase::AppendToBufferedData(CONST_CHAR_PTR data, uint32_t len)
{
    if (buffer_.empty()) {
        // no buffered data, use data directorly
        dataPtr_ = data;
        dataLen_ = len;
    } else {
        // append, then update ptr and length
        buffer_.append(data, len);
        dataPtr_ = buffer_.c_str();
        dataLen_ = buffer_.size();
    }
}

void AnalyzerBase::BufferRemainingData(uint32_t offset)
{
    if (offset < dataLen_) {
        buffer_ = std::string(dataPtr_ + offset, dataLen_ - offset);
    } else {
        // no data to buffer
        if (buffer_.empty()) {
            buffer_.clear();
        }
    }
}

int32_t AnalyzerBase::InitFrequency()
{
    std::string freq = Analysis::Dvvp::Common::Config::ConfigManager::instance()->GetFrequency();
    frequency_ = std::stod(freq) / 1000;   // 1000: mhz to ghz, syscnt * (1 / ghz) = ns
    if (frequency_ <= 0) {
        MSPROF_LOGE("init freqency failed. freq %s, frequency_ %f", freq.c_str(), frequency_);
        return PROFILING_FAILED;
    } else {
        MSPROF_EVENT("InitFrequency success. freqency: %f", frequency_);
        return PROFILING_SUCCESS;
    }
}

void AnalyzerBase::EraseRtMapByTaskId(uint32_t taskId, uint16_t streamId, std::map<std::string, RtOpInfo> &rtOpInfo)
{
}
 
void AnalyzerBase::EraseRtMapByStreamId(uint16_t streamId, std::map<std::string, RtOpInfo> &rtOpInfo)
{
}
 
void AnalyzerBase::HandleDeviceData(const std::string &key, const RtOpInfo &devData, uint32_t taskId,
    uint16_t streamId, uint32_t &time)
{
}
 
void AnalyzerBase::ConstructAndUploadOptimizeData(GeOpFlagInfo &opFlagData, RtOpInfo &rtTsOpdata)
{
}
 
uint32_t AnalyzerBase::GetGraphModelId(uint32_t modelId)
{
}
 
void AnalyzerBase::SetGraphModelId(uint32_t modelId, uint32_t graphId)
{
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
