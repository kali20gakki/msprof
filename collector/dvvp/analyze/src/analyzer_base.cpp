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
bool AnalyzerBase::isFftsPlus_ = false;
std::map<std::string, RtOpInfo> AnalyzerBase::rtOpInfo_;
std::map<std::string, RtOpInfo> AnalyzerBase::tsOpInfo_;
std::multimap<std::string, RtOpInfo> AnalyzerBase::tsTmpOpInfo_;
std::multimap<uint32_t, GeOpFlagInfo> AnalyzerBase::geNodeInfo_;
std::multimap<uint32_t, GeOpFlagInfo> AnalyzerBase::geApiInfo_;
std::multimap<uint32_t, GeOpFlagInfo> AnalyzerBase::geModelInfo_;
std::multimap<uint32_t, GeOpFlagInfo> AnalyzerBase::geOpInfo_;
std::multimap<uint32_t, GeOpFlagInfo> AnalyzerBase::geContextInfo_;
std::map<uint32_t, uint32_t> AnalyzerBase::graphIdMap_;
std::vector<ProfOpDesc> AnalyzerBase::opDescInfos_;
std::vector<RtOpInfo> AnalyzerBase::devTmpOpInfo_;
std::mutex AnalyzerBase::opDescInfoMtx_;
std::mutex AnalyzerBase::graphIdMtx_;
std::mutex AnalyzerBase::rtThreadMtx_;
std::mutex AnalyzerBase::geThreadMtx_;

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
        if (!buffer_.empty()) {
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
 
void AnalyzerBase::EraseRtMapByStreamId(uint16_t streamId, std::map<std::string, RtOpInfo> &rtOpInfo) const
{
    for (auto iter = rtOpInfo.begin(); iter != rtOpInfo.end();) {
        auto pos = iter->first.find(KEY_SEPARATOR);
        uint16_t iterStreamId = static_cast<uint16_t>(std::stoi(iter->first.substr(pos + 1)));
        if (iterStreamId != streamId) {
            iter++;
            continue;
        }
        rtOpInfo.erase(iter++);
    }
}
 
void AnalyzerBase::HandleDeviceData(const std::string &key, RtOpInfo &devData, uint32_t &time)
{
    std::unique_lock<std::mutex> lk(AnalyzerBase::rtThreadMtx_);
    if (devData.start >= devData.end) {
        MSPROF_LOGD("Device start and end error, start: %llu, end: %llu", devData.start, devData.end);
        AnalyzerBase::tsOpInfo_.erase(key);
        return;
    }
 
    if (AnalyzerBase::rtOpInfo_.empty()) {
        MSPROF_LOGI("rt opInfo is empty.");
        AnalyzerBase::tsTmpOpInfo_.insert(std::pair<std::string, RtOpInfo>(key, devData));
        AnalyzerBase::tsOpInfo_.erase(key);
        return;
    }

    auto hostIter = AnalyzerBase::rtOpInfo_.find(key);
    if (hostIter == AnalyzerBase::rtOpInfo_.end()) {
        MSPROF_LOGI("Device data not match runtime track.");
        AnalyzerBase::tsOpInfo_.erase(key);
        return;
    }

    devData.tsTrackTimeStamp = hostIter->second.tsTrackTimeStamp;
    devData.threadId = hostIter->second.threadId;

    time++;
    MSPROF_LOGD("Success to merge runtime track and Hwts|Ffts data. timeStamp: %llu, "
        "threadId: %u, taskId+streamId: %s, start: %llu, end: %llu, startAicore: %llu, endAicore: %llu, contextId: %u",
        hostIter->second.tsTrackTimeStamp, devData.threadId, key.c_str(), devData.start, devData.end,
        devData.startAicore, devData.endAicore, devData.contextId);
    HandleUploadData(key, devData);
}

void AnalyzerBase::HandleUploadData(const std::string &key, RtOpInfo &devData)
{
    std::unique_lock<std::mutex> lk(AnalyzerBase::geThreadMtx_);
    AnalyzerBase::devTmpOpInfo_.emplace_back(std::move(devData));
    if (AnalyzerBase::geOpInfo_.empty()) {
        MSPROF_LOGI("ge opInfo is empty.");
        AnalyzerBase::tsOpInfo_.erase(key);
        return;
    } else {
        AnalyzerBase::devTmpOpInfo_.pop_back();
    }

    auto threadGroup = AnalyzerBase::geOpInfo_.equal_range(devData.threadId);
    if (threadGroup.first != AnalyzerBase::geOpInfo_.end()) {
        for (auto geIter = threadGroup.first; geIter != threadGroup.second; ++geIter) {
            if (devData.tsTrackTimeStamp > geIter->second.end ||
                devData.tsTrackTimeStamp <= geIter->second.start) { // time include
                continue;
            }
            ConstructAndUploadOptimizeData(geIter->second, devData);
            break;
        }
    }

    AnalyzerBase::tsOpInfo_.erase(key);
}
 
void AnalyzerBase::ConstructAndUploadOptimizeData(GeOpFlagInfo &opFlagData, RtOpInfo &rtTsOpdata)
{
    ProfOpDesc opDesc;
    auto ret = memset_s(&opDesc, sizeof(ProfOpDesc), 0, sizeof(ProfOpDesc));
    if (ret != EOK) {
        MSPROF_LOGE("memset failed ret:%d", ret);
        return;
    }
    std::string opName;
    std::string opType;
    opDesc.modelId = GetGraphModelId(opFlagData.modelId);
    opName = HashData::instance()->GetHashInfo(opFlagData.opNameHash);
    opType = HashData::instance()->GetHashInfo(opFlagData.opTypeHash);
    uint64_t opIndex = OpDescParser::instance()->SetOpTypeAndOpName(opType, opName);
    if (opIndex == 0) {
        return;
    }
    opDesc.threadId = rtTsOpdata.threadId;
    opDesc.opIndex = opIndex;
    opDesc.duration = rtTsOpdata.end - rtTsOpdata.start;
    opDesc.start = rtTsOpdata.start;
    opDesc.end = rtTsOpdata.end;
    opDesc.flag = rtTsOpdata.flag;
    if (opType == "FFTS_PLUS") {
        opDesc.flag = ACL_SUBSCRIBE_SUBGRAPH;
    }
    opDesc.executionTime = rtTsOpdata.endAicore - rtTsOpdata.startAicore;
    opDesc.signature = analysis::dvvp::common::utils::Utils::GenerateSignature(
        reinterpret_cast<uint8_t *>(&opDesc) + sizeof(uint32_t), sizeof(ProfOpDesc) - sizeof(uint32_t));
    MSPROF_LOGD("Upload opt data push to vector. modelId: %u ,threadId: %u, duration: %llu, executionTime: %llu,"
        " opName: %s, opType: %s", opDesc.modelId, opDesc.threadId, opDesc.duration, opDesc.executionTime,
        opName.c_str(), opType.c_str());
    std::unique_lock<std::mutex> lk(opDescInfoMtx_);
    AnalyzerBase::opDescInfos_.emplace_back(std::move(opDesc));
}
 
uint32_t AnalyzerBase::GetGraphModelId(uint32_t modelId) const
{
    std::unique_lock<std::mutex> lk(graphIdMtx_);
    if (graphIdMap_.find(modelId) == graphIdMap_.end()) {
        return modelId;
    } else {
        return graphIdMap_[modelId];
    }
}
 
void AnalyzerBase::SetGraphModelId(uint32_t modelId, uint32_t graphId) const
{
    std::unique_lock<std::mutex> lk(graphIdMtx_);
    graphIdMap_[modelId] = graphId;
}

void AnalyzerBase::ClearAnalyzerData()
{
    AnalyzerBase::isFftsPlus_ = false;
    AnalyzerBase::rtOpInfo_.clear();
    AnalyzerBase::tsOpInfo_.clear();
    AnalyzerBase::geNodeInfo_.clear();
    AnalyzerBase::geApiInfo_.clear();
    AnalyzerBase::geModelInfo_.clear();
    AnalyzerBase::geOpInfo_.clear();
    AnalyzerBase::graphIdMap_.clear();
    AnalyzerBase::opDescInfos_.clear();
    AnalyzerBase::geContextInfo_.clear();
    MSPROF_LOGI("Analyzer OnOptimizeData clear all data.");
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
