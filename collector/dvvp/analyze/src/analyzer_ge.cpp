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
#include "proto/profiler.pb.h"
#include "transport/hash_data.h"

namespace Analysis {
namespace Dvvp {
namespace Analyze {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::transport;
static const uint32_t GE_TASK_DATA_SIZE = sizeof(struct MsprofGeProfTaskData);
static const int32_t GE_ID_MAP_SIZE = sizeof(struct MsprofGeProfIdMapData);

bool AnalyzerGe::IsGeData(const std::string &fileName)
{
    // Ge data starts with "Framework"
    if (fileName.find("Framework") != std::string::npos) {
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
    if (idMap_.find(modelId) == idMap_.end()) {
        return modelId;
    } else {
        return idMap_[modelId];
    }
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
    int32_t remaindLen = static_cast<int32_t>(len);
    for (; remaindLen >= GE_ID_MAP_SIZE; remaindLen -= GE_ID_MAP_SIZE, idMapData++) {
        if (idMapData->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM ||
            idMapData->dataTag != MSPROF_GE_DATA_TAG_ID_MAP) {
            MSPROF_LOGE("Check ge id map data fail. len:%u, magicNumber:%u, dataTag:%u",
                        len, idMapData->magicNumber, idMapData->dataTag);
            continue;
        }
        idMap_[idMapData->modelId] = idMapData->graphId;
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
        opInfo.opName = HashData::instance()->GetHashData("Framework", opName->data.hashId);
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
        opInfo.opType = HashData::instance()->GetHashData("Framework", opType->data.hashId);
    }
}

int32_t AnalyzerGe::ParseOpData(CONST_CHAR_PTR data)
{
    auto geTaskData = reinterpret_cast<const MsprofGeProfTaskData *>(data);
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
    MSPROF_EVENT("total_size_analyze, module: GE, analyzed %llu, total %llu, op info %u, idMap %u",
                 analyzedBytes_, totalBytes_, opInfos_.size(), idMap_.size());
}
}  // namespace Analyze
}  // namespace Dvvp
}  // namespace Analysis
