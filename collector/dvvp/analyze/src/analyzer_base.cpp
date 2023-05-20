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
    for (auto iter = rtOpInfo.begin(); iter != rtOpInfo.end();) {
        auto pos = iter->first.find(KEY_SEPARATOR);
        uint32_t iterTaskId = static_cast<uint32_t>(std::stoi(iter->first.substr(0, pos)));
        uint16_t iterStreamId = static_cast<uint16_t>(std::stoi(iter->first.substr(pos + 1)));
        if (iterStreamId != streamId) {
            iter++;
            continue;
        }
        if (iterTaskId > taskId) {
            iter++;
            continue;
        }
        rtOpInfo.erase(iter++);
    }
}
 
void AnalyzerBase::EraseRtMapByStreamId(uint16_t streamId, std::map<std::string, RtOpInfo> &rtOpInfo)
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
 
void AnalyzerBase::HandleDeviceData(const std::string &key, const RtOpInfo &devData, uint32_t taskId,
    uint16_t streamId, uint32_t &time)
{
    auto hostIter = AnalyzerBase::rtOpInfo_.find(key);
    if (hostIter == AnalyzerBase::rtOpInfo_.end()) {
        MSPROF_LOGW("invalid key %s for runtime op info.", key.c_str());
        return;
    }
    if (devData.start == 0 || devData.end == 0) { // incomplete device data
        return;
    }

    std::string mergeKey = std::to_string(hostIter->second.threadId);
    hostIter->second.start = devData.start;
    hostIter->second.end = devData.end;
    hostIter->second.startAicore = devData.startAicore;
    hostIter->second.endAicore = devData.endAicore;
    hostIter->second.flag = devData.flag;

    time++;
    MSPROF_LOGD("Success to merge runtime track and Hwts data in parsing Hwts data. "
        "threadId: %s, taskId+streamId: %s, start: %llu, end: %llu, startAicore: %llu, endAicore: %llu",
        mergeKey.c_str(), key.c_str(), devData.start, devData.end, devData.startAicore, devData.endAicore);
    EraseRtMapByTaskId(taskId, streamId, AnalyzerBase::tsOpInfo_); // del device data
    if (hostIter->second.ageFlag) {
        EraseRtMapByTaskId(taskId, streamId, AnalyzerBase::rtOpInfo_); // if age del host data
    }

    if (geOpInfo_.empty()) {
        MSPROF_LOGE("geOpInfo is empty");
        return;
    }

    auto threadGroup = geOpInfo_.equal_range(mergeKey);
    if (threadGroup.first != geOpInfo_.end()) {
        for (auto geIter = threadGroup.first; geIter != threadGroup.second; ++geIter) {
            if (hostIter->second.tsTrackTimeStamp >= geIter->second.end ||
                hostIter->second.tsTrackTimeStamp <= geIter->second.start) { // time include
                continue;
            }
            ConstructAndUploadOptimizeData(geIter->second, hostIter->second);
            return;
        }
    }
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
    opDescInfos_.emplace_back(std::move(opDesc));
}
 
uint32_t AnalyzerBase::GetGraphModelId(uint32_t modelId)
{
    std::unique_lock<std::mutex> lk(graphIdMtx_);
    if (graphIdMap_.find(modelId) == graphIdMap_.end()) {
        return modelId;
    } else {
        return graphIdMap_[modelId];
    }
}
 
void AnalyzerBase::SetGraphModelId(uint32_t modelId, uint32_t graphId)
{
    std::unique_lock<std::mutex> lk(graphIdMtx_);
    graphIdMap_[modelId] = graphId;
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
