/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: analyze ge data
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2020-10-24
 */
#include "analyzer_ge.h"

#include "nlohmann/json.hpp"

#include "data_struct.h"
#include "errno/error_code.h"
#include "msprof_dlog.h"
#include "transport/hash_data.h"
#include "config/config_manager.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;
using namespace Analysis::Dvvp::Common::Config;
static const int32_t GE_COMPACT_INFO_SIZE = sizeof(MsprofCompactInfo);
static const int32_t ADDITIONAL_INFO_SIZE = sizeof(MsprofAdditionalInfo);
static const int32_t GE_EVENT_SIZE = sizeof(MsprofEvent);
static const int32_t GE_API_SIZE = sizeof(MsprofApi);

bool AnalyzerGe::IsGeApiOrEventData(const std::string &fileName) const
{
    // Ge api data
    return (fileName.find("api_event") != std::string::npos) ? true : false;
}
 
bool AnalyzerGe::IsGeCompactData(const std::string &tag) const
{
    // Ge compact data
    return (tag.find("node_basic_info") != std::string::npos) ? true : false;
}
 
bool AnalyzerGe::IsGeGraphIdMapData(const std::string &tag) const
{
    // Ge compact data
    return (tag.find("graph_id_map") != std::string::npos) ? true : false;
}

bool AnalyzerGe::IsGeContextData(const std::string &tag) const
{
    // Ge context data
    return (tag.find("context_id_info") != std::string::npos) ? true : false;
}

bool AnalyzerGe::GetIsAllStaticShape() const
{
    return isAllStaticShape_;
}

bool AnalyzerGe::GetStreamType(const int &streamId, uint32_t &streamType)
{
    auto iter = steamState_.find(streamId);
    if (iter == steamState_.end()) {
        MSPROF_LOGI("Ge stream info is not ready");
        return false;
    } else {
        streamType = iter->second.streamType;
        return true;
    }
}

bool AnalyzerGe::IsOpInfoCompleted(const std::string &opId)
{
    return (opInfos_.find(opId) != opInfos_.end());
}

uint32_t AnalyzerGe::GetModelId(const std::string &opId) const
{
    auto iter = opInfos_.find(opId);
    return (iter == opInfos_.end() ? 0 : iter->second.modelId);
}

uint32_t AnalyzerGe::GetModelId(uint32_t modelId)
{
    return GetGraphModelId(modelId);
}

std::string AnalyzerGe::GetOpName(const std::string &opId)
{
    auto iter = opInfos_.find(opId);
    return (iter == opInfos_.end() ? std::string() : iter->second.opName);
}

std::string AnalyzerGe::GetOpType(const std::string &opId)
{
    auto iter = opInfos_.find(opId);
    return (iter == opInfos_.end() ? std::string() : iter->second.opType);
}

void AnalyzerGe::PrintStats() const
{
    MSPROF_EVENT("total_size_analyze, module: GE, analyzed %llu, total %llu, api time %u, node time %u, "
                 "event time %u, merge %u",
                 analyzedBytes_, totalBytes_, totalApiTimes_, totalNodeTimes_, totalEventTimes_, totalGeMerges_);
}

void AnalyzerGe::GeApiAndEventParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (fileChunkReq == nullptr) {
        MSPROF_LOGE("ge api and event parse message is null");
        return;
    }

    if (fileChunkReq->fileName.find("unaging") != std::string::npos) {
        totalBytes_ += fileChunkReq->chunkSize;
        ParseApiAndEventInfo(fileChunkReq->chunk.c_str(), fileChunkReq->chunkSize, false);
        return;
    }

    MSPROF_LOGD("Dropped ge data, fileName: %s", fileChunkReq->fileName.c_str());
}

void AnalyzerGe::ParseApiAndEventInfo(CONST_CHAR_PTR data, uint32_t len, bool ageFlag)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainLen = dataLen_ - offset;
        if (remainLen < GE_API_SIZE) {
            MSPROF_LOGW("ModelApiAndEventInfo remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }

        auto mlApiData = reinterpret_cast<const MsprofApi *>(dataPtr_ + offset);
        MSPROF_LOGD("ParseModelApiAndEvent level: %hu, type %u.", mlApiData->level, mlApiData->type);
        if (mlApiData->endTime != MSPROF_EVENT_FLAG && mlApiData->type == MSPROF_REPORT_NODE_LAUNCH_TYPE) {
            HandleApiInfo(dataPtr_ + offset, ageFlag);
            analyzedBytes_ += GE_API_SIZE;
            totalApiTimes_++;
        } else if (mlApiData->endTime == MSPROF_EVENT_FLAG && mlApiData->level == MSPROF_REPORT_MODEL_LEVEL &&
            mlApiData->type == MSPROF_REPORT_MODEL_LOAD_TYPE) {
            HandleModelInfo(dataPtr_ + offset, ageFlag);
            analyzedBytes_ += GE_EVENT_SIZE;
            totalEventTimes_++;
        }
        offset += GE_API_SIZE;
    }
    BufferRemainingData(offset);
    MatchApiInfo(AnalyzerBase::geApiInfo_, AnalyzerBase::geModelInfo_, AnalyzerBase::geNodeInfo_,
        AnalyzerBase::geContextInfo_);
}

void AnalyzerGe::HandleApiInfo(CONST_CHAR_PTR data, bool ageFlag) const
{
    auto klData = reinterpret_cast<const MsprofApi *>(data);
    uint32_t key = klData->threadId;
    GeOpFlagInfo opInfo{0, 0, 0, klData->beginTime, klData->endTime, false, false, ageFlag, UINT16_MAX};
    AnalyzerBase::geApiInfo_.insert(std::pair<uint32_t, GeOpFlagInfo>(key, opInfo));
    MSPROF_LOGD("Insert to GeApiInfo, key: %u, beginTime: %llu, endTime: %llu.",
        key, klData->beginTime, klData->endTime);
}

void AnalyzerGe::HandleModelInfo(CONST_CHAR_PTR data, bool ageFlag) const
{
    auto mlData = reinterpret_cast<const MsprofEvent *>(data);
    uint32_t key = mlData->threadId;
    if (AnalyzerBase::geModelInfo_.count(key) == 0) {
        GeOpFlagInfo opInfo = {0, 0, mlData->itemId, mlData->timeStamp, 0, false, false, ageFlag, UINT16_MAX};
        AnalyzerBase::geModelInfo_.insert(std::pair<uint32_t, GeOpFlagInfo>(key, opInfo));
        MSPROF_LOGD("Insert start: %llu, modelId: %llu to geModelInfo map cause no same key.",
            mlData->timeStamp, mlData->itemId);
        return;
    }

    bool modelIdExist = false;
    for (auto iter = AnalyzerBase::geModelInfo_.begin(); iter != AnalyzerBase::geModelInfo_.end(); iter++) {
        if (iter->second.modelId != mlData->itemId || iter->first != key) { // same modelId and same threadId
            continue;
        }
        if (iter->second.end != 0) {
            iter->second.end = 0;
            iter->second.start = mlData->timeStamp; // repeat modelId and threadId
            MSPROF_LOGD("Repeat Insert start: %llu, modelId: %llu to geModelInfo map.", mlData->timeStamp,
                mlData->itemId);
            return;
        }
        if (iter->second.start >= mlData->timeStamp) { // start end error
            MSPROF_LOGE("Model info: op start latter than op end.");
            return;
        }
        iter->second.end = mlData->timeStamp;
        MSPROF_LOGD("Insert end: %llu, modelId: %llu to geModelInfo map.", mlData->timeStamp, mlData->itemId);
        modelIdExist = true;
    }

    if (!modelIdExist) { // same threadId different modelid
        GeOpFlagInfo opInfo{0, 0, mlData->itemId, mlData->timeStamp, 0, false, false, ageFlag, UINT16_MAX};
        AnalyzerBase::geModelInfo_.insert(std::pair<uint32_t, GeOpFlagInfo>(key, opInfo));
        MSPROF_LOGD("Insert start: %llu, modelId: %llu to geModelInfo_ cause no same modelId.", mlData->timeStamp,
            mlData->itemId);
    }
}

void AnalyzerGe::GeCompactParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (fileChunkReq == nullptr) {
        MSPROF_LOGE("ge compact parse message is null");
        return;
    }

    if (fileChunkReq->fileName.find("node_basic_info") != std::string::npos) {
        totalBytes_ += fileChunkReq->chunkSize;
        ParseNodeBasicInfo(fileChunkReq->chunk.c_str(), fileChunkReq->chunkSize);
        return;
    }

    MSPROF_LOGD("Dropped ge data, fileName: %s", fileChunkReq->fileName.c_str());
}

void AnalyzerGe::ParseNodeBasicInfo(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainLen = dataLen_ - offset;
        if (remainLen < GE_COMPACT_INFO_SIZE) {
            MSPROF_LOGW("NodeBasicInfo remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }

        auto nodeData = reinterpret_cast<const MsprofCompactInfo *>(dataPtr_ + offset);
        MSPROF_LOGD("ParseNodeBasicInfo level: %hu.", nodeData->level);
        if (nodeData->level == MSPROF_REPORT_NODE_LEVEL) {
            HandleNodeBasicInfo(dataPtr_ + offset);
            analyzedBytes_ += GE_COMPACT_INFO_SIZE;
            totalNodeTimes_++;
        }

        offset += GE_COMPACT_INFO_SIZE;
    }
    BufferRemainingData(offset);
    MatchApiInfo(AnalyzerBase::geApiInfo_, AnalyzerBase::geModelInfo_, AnalyzerBase::geNodeInfo_,
                 AnalyzerBase::geContextInfo_);
}

void AnalyzerGe::HandleNodeBasicInfo(CONST_CHAR_PTR data) const
{
    auto compactData = reinterpret_cast<const MsprofCompactInfo *>(data);
    auto nodeData = compactData->data.nodeBasicInfo;
    uint32_t key = compactData->threadId;
    uint64_t opNameHash = nodeData.opName;
    uint64_t opTypeHash = nodeData.opType;
    uint64_t timeStamp = compactData->timeStamp;
    std::string nodeName = HashData::instance()->GetHashInfo(opNameHash);
    std::string nodeType = HashData::instance()->GetHashInfo(opTypeHash);
    if (!AnalyzerBase::isFftsPlus_ && nodeType == "FFTS_PLUS") {
        AnalyzerBase::isFftsPlus_ = true;
        MSPROF_LOGI("Ffts plus mode on");
        return;
    }
    GeOpFlagInfo opInfo{opNameHash, opTypeHash, 0, timeStamp, 0, false, false, false, UINT16_MAX};
    AnalyzerBase::geNodeInfo_.insert(std::pair<uint32_t, GeOpFlagInfo>(key, opInfo));
    MSPROF_LOGD("insert NodeInfo timeStamp: %llu, opName: %s, opType: %s",
                timeStamp, nodeName.c_str(), nodeType.c_str());
}

void AnalyzerGe::GeGraphIdMapParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (fileChunkReq == nullptr) {
        MSPROF_LOGE("ge compact parse message is null");
        return;
    }

    if (fileChunkReq->fileName.find("unaging") != std::string::npos) {
        totalBytes_ += fileChunkReq->chunkSize;
        ParseGraphIdMap(fileChunkReq->chunk.c_str(), fileChunkReq->chunkSize);
        return;
    }
    MSPROF_LOGD("Dropped ge data, fileName: %s", fileChunkReq->fileName.c_str());
}

void AnalyzerGe::ParseGraphIdMap(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainLen = dataLen_ - offset;
        if (remainLen < ADDITIONAL_INFO_SIZE) {
            MSPROF_LOGW("ParseGraphIdMap remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }

        auto graphIdMapData = reinterpret_cast<const MsprofAdditionalInfo *>(dataPtr_ + offset);
        auto graphIdInfo = reinterpret_cast<const MsprofGraphIdInfo *>(graphIdMapData->data);
        if (graphIdMapData->level == MSPROF_REPORT_MODEL_LEVEL &&
            graphIdInfo->graphId != std::numeric_limits<uint32_t>::max()) {
            SetGraphModelId(graphIdInfo->modelId, graphIdInfo->graphId);
            MSPROF_LOGD("ParseGraphIdMap graph id %u, model id: %u.", graphIdInfo->graphId, graphIdInfo->modelId);
        }
        offset += ADDITIONAL_INFO_SIZE;
    }
    BufferRemainingData(offset);
}

void AnalyzerGe::GeContextParse(SHARED_PTR_ALIA<analysis::dvvp::ProfileFileChunk> fileChunkReq)
{
    if (fileChunkReq == nullptr) {
        MSPROF_LOGE("ge context parse message is null");
        return;
    }

    totalBytes_ += fileChunkReq->chunkSize;
    ParseContextIdInfo(fileChunkReq->chunk.c_str(), fileChunkReq->chunkSize);
}

void AnalyzerGe::ParseContextIdInfo(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainLen = dataLen_ - offset;
        if (remainLen < ADDITIONAL_INFO_SIZE) {
            MSPROF_LOGW("ContextIdInfo remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }

        auto contextData = reinterpret_cast<const MsprofAdditionalInfo *>(dataPtr_ + offset);
        if (contextData->level == MSPROF_REPORT_NODE_LEVEL &&
            contextData->type == MSPROF_REPORT_NODE_CONTEXT_ID_INFO_TYPE) {
            HandleContextIdInfo(dataPtr_ + offset);
            analyzedBytes_ += ADDITIONAL_INFO_SIZE;
        }

        offset += ADDITIONAL_INFO_SIZE;
    }
    BufferRemainingData(offset);
    MatchApiInfo(AnalyzerBase::geApiInfo_, AnalyzerBase::geModelInfo_, AnalyzerBase::geNodeInfo_,
        AnalyzerBase::geContextInfo_);
}

void AnalyzerGe::HandleContextIdInfo(CONST_CHAR_PTR data) const
{
    auto AdditionalData = reinterpret_cast<const MsprofAdditionalInfo *>(data);
    auto contextIdInfo = reinterpret_cast<const MsprofContextIdInfo *>(AdditionalData->data);
    uint32_t key = AdditionalData->threadId;
    GeOpFlagInfo opInfo{contextIdInfo->opName, 0, 0, AdditionalData->timeStamp, 0, false, false, false,
        static_cast<uint16_t>(contextIdInfo->ctxIds[0])};
    AnalyzerBase::geContextInfo_.insert(std::pair<uint32_t, GeOpFlagInfo>(key, opInfo));
    MSPROF_LOGD("insert ContextIdInfo opName: %s, contextId: %u, num: %u.",
                (HashData::instance()->GetHashInfo(contextIdInfo->opName)).c_str(),
                contextIdInfo->ctxIds[0], contextIdInfo->ctxIdNum);
}

bool AnalyzerGe::HandleContextWithNode(std::multimap<uint32_t, GeOpFlagInfo> &nodeInfo,
    std::multimap<uint32_t, GeOpFlagInfo> &contextInfo) const
{
    if (contextInfo.empty() || nodeInfo.empty()) {
        return false;
    }

    for (auto NodeIter = nodeInfo.rbegin(); NodeIter != nodeInfo.rend(); NodeIter++) {
        for (auto cxtIter = contextInfo.begin(); cxtIter != contextInfo.end(); cxtIter++) {
            if (NodeIter->first == cxtIter->first &&
                NodeIter->second.start == cxtIter->second.start &&
                NodeIter->second.opNameHash == cxtIter->second.opNameHash) {
                NodeIter->second.contextId = cxtIter->second.contextId;
            }
        }
    }

    return true;
}

void AnalyzerGe::MatchApiInfoByModelInfo(const uint32_t &threadId, struct GeOpFlagInfo &info,
    std::multimap<uint32_t, GeOpFlagInfo> &modelInfo) const
{
    for (auto model = modelInfo.begin(); model != modelInfo.end(); model++) {
        if (model->first == threadId &&
            info.start > model->second.start &&
            info.end < model->second.end &&
            info.ageFlag == model->second.ageFlag) { // match modelid
            info.modelId = model->second.modelId;
            info.modelFlag = true;
            MSPROF_LOGD("model match threadId: %u, modelId: %llu", model->first, model->second.modelId);
            break;
        }
    }
    return;
}

void AnalyzerGe::MatchApiInfoByNodeInfo(const uint32_t &threadId, struct GeOpFlagInfo &info,
    std::multimap<uint32_t, GeOpFlagInfo> &nodeInfo)
{
    for (auto node = nodeInfo.begin(); node != nodeInfo.end();) {
        if (node->first == threadId &&
            node->second.start >= info.start &&
            node->second.start <= info.end) { // match node
            info.opNameHash = node->second.opNameHash;
            info.opTypeHash = node->second.opTypeHash;
            info.nodeFlag = true;
            info.contextId = node->second.contextId;
            nodeInfo.erase(node++);
            std::string nodeName = HashData::instance()->GetHashInfo(info.opNameHash);
            MSPROF_LOGD("Success to match ge opinfo data and insert in map."
                "Key: %u, start: %llu, end: %llu, name: %s, context: %u.", threadId,
                info.start, info.end, nodeName.c_str(), info.contextId);
            AnalyzerBase::geOpInfo_.insert(std::pair<uint32_t, GeOpFlagInfo>(threadId, info));
            totalGeMerges_++;
            if (!AnalyzerBase::isFftsPlus_) {
                break;
            }
        } else {
            node++;
        }
    }
    return;
}

void AnalyzerGe::MatchApiInfo(std::multimap<uint32_t, GeOpFlagInfo> &apiInfo,
    std::multimap<uint32_t, GeOpFlagInfo> &modelInfo,
    std::multimap<uint32_t, GeOpFlagInfo> &nodeInfo,
    std::multimap<uint32_t, GeOpFlagInfo> &contextInfo)
{
    if (apiInfo.empty()) {
        return;
    }

    if (ConfigManager::instance()->GetPlatformType() == PlatformType::CHIP_V4_1_0 &&
        AnalyzerBase::isFftsPlus_) {
        if (!HandleContextWithNode(nodeInfo, contextInfo)) {
            return;
        }
    }
    std::unique_lock<std::mutex> lk(AnalyzerBase::geThreadMtx_);
    for (auto api = apiInfo.begin(); api != apiInfo.end();) {
        if (!api->second.modelFlag) {
            MatchApiInfoByModelInfo(api->first, api->second, modelInfo);
        }
        if (!api->second.modelFlag) {
            api++;
            continue;
        }
        MatchApiInfoByNodeInfo(api->first, api->second, nodeInfo);
        if (api->second.nodeFlag) {
            apiInfo.erase(api++);
        } else {
            api++;
        }
    }
    MatchDeviceOpInfo(AnalyzerBase::devTmpOpInfo_, AnalyzerBase::geOpInfo_);
}

void AnalyzerGe::MatchDeviceOpInfo(std::vector<RtOpInfo> &devTmpOpInfo,
    std::multimap<uint32_t, GeOpFlagInfo> &geOpInfo)
{
    if (devTmpOpInfo.empty() || geOpInfo.empty()) {
        return;
    }
    for (auto &it : devTmpOpInfo) {
        auto threadGroup = geOpInfo.equal_range(it.threadId);
        if (threadGroup.first == geOpInfo.end()) {
            continue;
        }
        for (auto geIter = threadGroup.first; geIter != threadGroup.second; ++geIter) {
            if (it.tsTrackTimeStamp > geIter->second.end ||
                it.tsTrackTimeStamp <= geIter->second.start) { // time include
                continue;
            }
            ConstructAndUploadOptimizeData(geIter->second, it);
            break;
        }
    }
    devTmpOpInfo.clear();
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
