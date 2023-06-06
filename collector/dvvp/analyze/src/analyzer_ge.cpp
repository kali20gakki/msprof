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
#include "message/codec.h"
#include "msprof_dlog.h"
#include "proto/msprofiler.pb.h"
#include "transport/hash_data.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;
static const uint32_t GE_TASK_DATA_SIZE = sizeof(struct MsprofGeProfTaskData);
static const int32_t GE_ID_MAP_SIZE = sizeof(struct MsprofGeProfIdMapData);
static const int32_t GE_COMPACT_INFO_SIZE = sizeof(MsprofCompactInfo);
static const int32_t ADDITIONAL_INFO_SIZE = sizeof(MsprofAdditionalInfo);
static const int32_t GE_EVENT_SIZE = sizeof(MsprofEvent);
static const int32_t GE_API_SIZE = sizeof(MsprofApi);

bool AnalyzerGe::IsGeData(const std::string &fileName)
{
    // Ge data starts with "Framework"
    if (fileName.find("Framework") != std::string::npos) {
        return true;
    }
    return false;
}

bool AnalyzerGe::IsGeApiData(const std::string &fileName)
{
    // Ge api data
    if (fileName.find("api") != std::string::npos) {
        return true;
    }
    return false;
}
 
bool AnalyzerGe::IsGeEventData(const std::string &fileName)
{
    // Ge event data
    if (fileName.find("event") != std::string::npos) {
        return true;
    }
    return false;
}
 
bool AnalyzerGe::IsGeCompactData(const std::string &tag)
{
    // Ge compact data
    if (tag.find("node_basic_info") != std::string::npos) {
        return true;
    }
    return false;
}
 
bool AnalyzerGe::IsGeGraphIdMapData(const std::string &tag)
{
    // Ge compact data
    if (tag.find("graph_id_map") != std::string::npos) {
        return true;
    }
    return false;
}

void AnalyzerGe::Parse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        MSPROF_LOGE("ge parse message is null");
        return;
    }
    if (message->tag().find("id_map_info") != std::string::npos) {
        totalBytes_ += static_cast<uint64_t>(message->chunksizeinbytes());
        if (message->chunksizeinbytes() < GE_ID_MAP_SIZE) {
            MSPROF_LOGE("id_map_info is incomplete data");
            return;
        }
        ParseIdMap(message->chunk().c_str(), message->chunksizeinbytes());
        return;
    }
    if (message->tag().find("task_desc_info") != std::string::npos) {
        totalBytes_ += static_cast<uint64_t>(message->chunksizeinbytes());
        ParseTaskDesc(message->chunk().c_str(), message->chunksizeinbytes());
        return;
    }
    MSPROF_LOGD("Dropped ge data, tag: %s", message->tag().c_str());
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

void AnalyzerGe::ParseIdMap(CONST_CHAR_PTR data, uint32_t len)
{
    auto idMapData = reinterpret_cast<const MsprofGeProfIdMapData *>(data);
    if (idMapData == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return;
    }
    int32_t remaindLen = static_cast<int32_t>(len);
    for (; remaindLen >= GE_ID_MAP_SIZE; remaindLen -= GE_ID_MAP_SIZE, idMapData++) {
        if (idMapData->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM ||
            idMapData->dataTag != MSPROF_GE_DATA_TAG_ID_MAP) {
            MSPROF_LOGE("Check ge id map data fail. len:%u, magicNumber:%u, dataTag:%u",
                        len, idMapData->magicNumber, idMapData->dataTag);
            continue;
        }
        SetGraphModelId(idMapData->modelId, idMapData->graphId);
        analyzedBytes_ += GE_ID_MAP_SIZE;
    }
    if (remaindLen != 0) {
        MSPROF_LOGW("ge id map data len error. len:%u, remaindLen:%d", len, remaindLen);
    }
}

void AnalyzerGe::ParseTaskDesc(CONST_CHAR_PTR data, uint32_t len)
{
    AppendToBufferedData(data, len);
    uint32_t parsedOpNum = 0;
    uint32_t parsedLen = 0;
    while (dataPtr_ != nullptr && parsedLen < dataLen_) {
        uint32_t remainLen = dataLen_ - parsedLen;
        if (remainLen < GE_TASK_DATA_SIZE) {
            // remaining is less then GE_TASK_DATA_SIZE, cache it to buffer
            MSPROF_LOGI("Ge remains %u bytes unparsed, cache it", remainLen);
            break;
        }
        if (ParseOpData(dataPtr_ + parsedLen) != PROFILING_SUCCESS) {
            MSPROF_LOGE("ParseOpData failed");
            break;
        }
        parsedOpNum += 1;
        parsedLen += GE_TASK_DATA_SIZE;
    }
    MSPROF_LOGI("Finish parsing ge data, BuffLen:%u NewDataLen:%u parsedLen:%u TotalOpNum:%u ParsedOp:%u",
                dataLen_, len, parsedLen, opInfos_.size(), parsedOpNum);
    BufferRemainingData(parsedLen);
}

void AnalyzerGe::ParseOpName(const MsprofGeProfTaskData &data, struct GeOpInfo &opInfo) const
{
    MsprofMixData *opName = const_cast<MsprofMixData *>(&data.opName);
    if (opName->type == MSPROF_MIX_DATA_STRING) {
        uint32_t dataStrLen = strnlen(opName->data.dataStr, MSPROF_MIX_DATA_STRING_LEN);
        if (dataStrLen == MSPROF_MIX_DATA_STRING_LEN) {
            MSPROF_LOGE("opName data len:%u over max length %u", dataStrLen, MSPROF_MIX_DATA_STRING_LEN - 1);
            opName->data.dataStr[MSPROF_MIX_DATA_STRING_LEN - 1] = 0;
        }
        opInfo.opName = opName->data.dataStr;
    } else {
        opInfo.opName = HashData::instance()->GetHashInfo(opName->data.hashId);
    }
}

void AnalyzerGe::ParseOpType(const MsprofGeProfTaskData &data, struct GeOpInfo &opInfo) const
{
    MsprofGeOpType *opType = const_cast<MsprofGeOpType *>(&data.opType);
    if (opType->type == MSPROF_MIX_DATA_STRING) {
        uint32_t dataStrLen = strnlen(opType->data.dataStr, MSPROF_GE_OP_TYPE_LEN);
        if (dataStrLen == MSPROF_GE_OP_TYPE_LEN) {
            MSPROF_LOGE("opType data len:%u over max length %u", dataStrLen, MSPROF_GE_OP_TYPE_LEN - 1);
            opType->data.dataStr[MSPROF_GE_OP_TYPE_LEN - 1] = 0;
        }
        opInfo.opType = opType->data.dataStr;
    } else {
        opInfo.opType = HashData::instance()->GetHashInfo(opType->data.hashId);
    }
}

int32_t AnalyzerGe::ParseOpData(CONST_CHAR_PTR data)
{
    auto geTaskData = reinterpret_cast<const MsprofGeProfTaskData *>(data);
    if (geTaskData == nullptr) {
        MSPROF_LOGE("Failed to call reinterpret_cast.");
        return PROFILING_FAILED;
    }
    if (geTaskData->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM ||
        geTaskData->dataTag != MSPROF_GE_DATA_TAG_TASK ||
        geTaskData->opName.type > MSPROF_MIX_DATA_STRING ||
        geTaskData->opType.type > MSPROF_MIX_DATA_STRING) {
        MSPROF_LOGE("Check ge op data fail. magicNumber:%u, dataTag:%u, opName:%u, opType:%u",
            geTaskData->magicNumber, geTaskData->dataTag,
            geTaskData->opName.type, geTaskData->opType.type);
        return PROFILING_FAILED;
    }

    GeOpInfo opInfo = {"", "", "", 0};
    std::string taskId = std::to_string(geTaskData->taskId);
    std::string streamId = std::to_string(geTaskData->streamId);
    std::string iterId = std::to_string(geTaskData->curIterNum);
    std::string contextId = std::to_string(geTaskData->contextId);
    opInfo.opId = taskId + KEY_SEPARATOR + streamId + KEY_SEPARATOR + contextId + KEY_SEPARATOR + iterId;
    opInfo.modelId = GetModelId(geTaskData->modelId);
    ParseOpName(*geTaskData, opInfo);
    ParseOpType(*geTaskData, opInfo);
    opInfos_.insert(std::make_pair(opInfo.opId, opInfo));
    analyzedBytes_ += GE_TASK_DATA_SIZE;

    return PROFILING_SUCCESS;
}

void AnalyzerGe::PrintStats()
{
    MSPROF_EVENT("total_size_analyze, module: GE, analyzed %llu, total %llu, api time %u, node time %u, "
                 "event time %u, merge %u",
                 analyzedBytes_, totalBytes_, totalApiTimes_, totalNodeTimes_, totalEventTimes_, totalGeMerges_);
}

void AnalyzerGe::GeApiParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        MSPROF_LOGE("ge api parse message is null");
        return;
    }

    if (message->filename().find("unaging") != std::string::npos) {
        totalBytes_ += message->chunksizeinbytes();
        ParseApiInfo(message->chunk().c_str(), message->chunksizeinbytes(), false);
        return;
    }

    MSPROF_LOGD("Dropped ge data, tag: %s", message->tag().c_str());
}

void AnalyzerGe::ParseApiInfo(CONST_CHAR_PTR data, uint32_t len, bool ageFlag)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainLen = dataLen_ - offset;
        if (remainLen < GE_API_SIZE) {
            MSPROF_LOGW("ModelApiInfo remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }

        auto mlApiData = reinterpret_cast<const MsprofApi *>(dataPtr_ + offset);
        MSPROF_LOGD("ParseModelApi level: %hu, type %u.", mlApiData->level, mlApiData->type);
        if (mlApiData->type == MSPROF_REPORT_NODE_LAUNCH_TYPE ||
            mlApiData->type == MSPROF_REPORT_NODE_LAUNCH_GE_TYPE) {
            HandleApiInfo(dataPtr_ + offset, ageFlag);
            analyzedBytes_ += GE_API_SIZE;
            totalApiTimes_++;
        }

        offset += GE_API_SIZE;
    }
    BufferRemainingData(offset);
    MatchApiInfo(AnalyzerBase::geApiInfo_, AnalyzerBase::geModelInfo_, AnalyzerBase::geNodeInfo_);
}

void AnalyzerGe::HandleApiInfo(CONST_CHAR_PTR data, bool ageFlag)
{
    auto klData = reinterpret_cast<const MsprofApi *>(data);
    std::string key = std::to_string(klData->threadId);
    GeOpFlagInfo opInfo{0, 0, 0, klData->beginTime, klData->endTime, false, false, ageFlag};
    AnalyzerBase::geApiInfo_.insert(std::pair<std::string, GeOpFlagInfo>(key, opInfo));
    MSPROF_LOGD("Insert to GeApiInfo, key: %s, beginTime: %llu, endTime: %llu.",
        key.c_str(), klData->beginTime, klData->endTime);
}

void AnalyzerGe::GeEventParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        MSPROF_LOGE("ge event parse message is null");
        return;
    }

    if (message->filename().find("unaging") != std::string::npos) {
        totalBytes_ += message->chunksizeinbytes();
        ParseModelInfo(message->chunk().c_str(), message->chunksizeinbytes(), false);
        return;
    }

    MSPROF_LOGD("Dropped ge data, tag: %s", message->tag().c_str());
}

void AnalyzerGe::ParseModelInfo(CONST_CHAR_PTR data, uint32_t len, bool ageFlag)
{
    AppendToBufferedData(data, len);
    uint32_t offset = 0;
    while (dataPtr_ != nullptr && offset < dataLen_) {
        uint32_t remainLen = dataLen_ - offset;
        if (remainLen < GE_EVENT_SIZE) {
            MSPROF_LOGW("ModelLoadInfo remains %u bytes unparsed, which is incomplete data", remainLen);
            break;
        }

        auto mlData = reinterpret_cast<const MsprofEvent *>(dataPtr_ + offset);
        MSPROF_LOGD("ParseModelLoadInfo level: %hu, type: %u", mlData->level, mlData->type);
        if (mlData->level == MSPROF_REPORT_MODEL_LEVEL &&
            mlData->type == MSPROF_REPORT_MODEL_LOAD_TYPE) {
            HandleModelInfo(dataPtr_ + offset, ageFlag);
            analyzedBytes_ += GE_EVENT_SIZE;
            totalEventTimes_++;
        }

        offset += GE_EVENT_SIZE;
    }
    BufferRemainingData(offset);
    MatchApiInfo(AnalyzerBase::geApiInfo_, AnalyzerBase::geModelInfo_, AnalyzerBase::geNodeInfo_);
}

void AnalyzerGe::HandleModelInfo(CONST_CHAR_PTR data, bool ageFlag)
{
    auto mlData = reinterpret_cast<const MsprofEvent *>(data);
    std::string key = std::to_string(mlData->threadId);
    if (AnalyzerBase::geModelInfo_.count(key) == 0) {
        GeOpFlagInfo opInfo = {0, 0, mlData->itemId, mlData->timeStamp, 0, false, false, ageFlag};
        AnalyzerBase::geModelInfo_.insert(std::pair<std::string, GeOpFlagInfo>(key, opInfo));
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
        GeOpFlagInfo opInfo{0, 0, mlData->itemId, mlData->timeStamp, 0, false, false, ageFlag};
        AnalyzerBase::geModelInfo_.insert(std::pair<std::string, GeOpFlagInfo>(key, opInfo));
        MSPROF_LOGD("Insert start: %llu, modelId: %llu to geModelInfo_ cause no same modelId.", mlData->timeStamp,
            mlData->itemId);
    }
}

void AnalyzerGe::GeCompactParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        MSPROF_LOGE("ge compact parse message is null");
        return;
    }

    if (message->tag().find("node_basic_info") != std::string::npos) {
        totalBytes_ += message->chunksizeinbytes();
        ParseNodeBasicInfo(message->chunk().c_str(), message->chunksizeinbytes());
        return;
    }

    MSPROF_LOGD("Dropped ge data, tag: %s", message->tag().c_str());
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
    MatchApiInfo(AnalyzerBase::geApiInfo_, AnalyzerBase::geModelInfo_, AnalyzerBase::geNodeInfo_);
}

void AnalyzerGe::HandleNodeBasicInfo(CONST_CHAR_PTR data)
{
    auto compactData = reinterpret_cast<const MsprofCompactInfo *>(data);
    auto nodeData = compactData->data.nodeBasicInfo;
    std::string key = std::to_string(compactData->threadId);
    uint64_t opNameHash = nodeData.opName;
    uint64_t opTypeHash = nodeData.opType;
    uint64_t timeStamp = compactData->timeStamp;
    GeOpFlagInfo opInfo{opNameHash, opTypeHash, 0, timeStamp, 0, false, false, false};
    AnalyzerBase::geNodeInfo_.insert(std::pair<std::string, GeOpFlagInfo>(key, opInfo));
    MSPROF_LOGD("insert NodeInfo timeStamp: %llu.", timeStamp);
}

void AnalyzerGe::GeGraphIdMapParse(SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> message)
{
    if (message == nullptr) {
        MSPROF_LOGE("ge compact parse message is null");
        return;
    }

    totalBytes_ += message->chunksizeinbytes();
    ParseGraphIdMap(message->chunk().c_str(), message->chunksizeinbytes());
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

void AnalyzerGe::MatchApiInfoByModelInfo(const std::string &threadId, struct GeOpFlagInfo &info,
    std::multimap<std::string, GeOpFlagInfo> &modelInfo)
{
    for (auto model = modelInfo.begin(); model != modelInfo.end(); model++) {
        if (model->first == threadId &&
            info.start > model->second.start &&
            info.end < model->second.end &&
            info.ageFlag == model->second.ageFlag) { // match modelid
            info.modelId = model->second.modelId;
            info.modelFlag = true;
            MSPROF_LOGD("model match threadId: %s, modelId: %llu", model->first.c_str(), model->second.modelId);
            break;
        }
    }
    return;
}

void AnalyzerGe::MatchApiInfo(std::multimap<std::string, GeOpFlagInfo> &apiInfo,
    std::multimap<std::string, GeOpFlagInfo> &modelInfo,
    std::multimap<std::string, GeOpFlagInfo> &nodeInfo)
{
    if (apiInfo.empty() || nodeInfo.empty() || modelInfo.empty()) {
        return;
    }

    for (auto api = apiInfo.begin(); api != apiInfo.end();) {
        MSPROF_LOGD("api match threadId: %s", api->first.c_str());
        if (!api->second.modelFlag) {
            MatchApiInfoByModelInfo(api->first, api->second, modelInfo);
        }

        if (!api->second.modelFlag) {
            api++;
            continue;
        }
        for (auto node = nodeInfo.begin(); node != nodeInfo.end(); node++) {
            if (node->first == api->first &&
                node->second.start >= api->second.start &&
                node->second.start <= api->second.end) { // match node
                api->second.opNameHash = node->second.opNameHash;
                api->second.opTypeHash = node->second.opTypeHash;
                api->second.nodeFlag = true;
                MSPROF_LOGD("node match threadId: %s", node->first.c_str());
                nodeInfo.erase(node++);
                break;
            }
        }
        if (api->second.nodeFlag && api->second.modelFlag) {
            MSPROF_LOGD("Success to match ge opinfo data and insert in map."
                "Key: %s, start: %llu, end: %llu, name: %llu.", api->first.c_str(),
                api->second.start, api->second.end, api->second.opNameHash);
            AnalyzerBase::geOpInfo_.insert(std::pair<std::string, GeOpFlagInfo>(api->first, api->second));
            totalGeMerges_++;
            apiInfo.erase(api++);
        } else {
            api++;
        }
    }
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
