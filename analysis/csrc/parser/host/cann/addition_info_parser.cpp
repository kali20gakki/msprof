/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : addition_info_parser.cpp
 * Description        : addition数据解析
 * Author             : msprof team
 * Creation Date      : 2023/11/18
 * *****************************************************************************
 */

#include "addition_info_parser.h"

#include "chunk_generator.h"
#include "log.h"
#include "utils.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
using namespace Analysis::Utils;

void AdditionInfoParser::Init(const std::vector<std::string> &filePrefix)
{
    chunkProducer_ = MakeShared<ChunkGenerator>(sizeof(MsprofAdditionalInfo), path_, filePrefix);
    chunkConsumers_ = {
        {typeid(MsprofAdditionalInfo).hash_code(), chunkProducer_},
    };
}

int AdditionInfoParser::ConsumeChunk(std::shared_ptr<void> &chunk, const std::shared_ptr<ChunkGenerator> &chunkConsumer)
{
    chunk = chunkConsumer->Pop();
    if (!chunk) {
        ERROR("chunk is null.");
        return ANALYSIS_ERROR;
    }
    auto data = ChunkGenerator::ToObj<MsprofAdditionalInfo>(chunk);
    if (data->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
        ERROR("Check 5A5A failed in place &.", chunkConsumer->Size());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

TensorInfoParser::TensorInfoParser(const std::string &path) : AdditionInfoParser(path)
{
    chunkProducer_ = MakeShared<ChunkGenerator>(sizeof(MsprofAdditionalInfo), path_, filePrefix_);
    chunkConsumers_ = {
        {typeid(ConcatTensorInfo).hash_code(), MakeShared<ChunkGenerator>(sizeof(ConcatTensorInfo))},
    };
}

std::shared_ptr<ConcatTensorInfo> TensorInfoParser::CreateConcatTensorInfo(
    const std::shared_ptr<MsprofAdditionalInfo> &additionalInfo)
{
    std::shared_ptr<ConcatTensorInfo> concatTensorInfo = MakeShared<ConcatTensorInfo>();
    if (!concatTensorInfo || !additionalInfo) {
        return nullptr;
    }
    concatTensorInfo->level = additionalInfo->level;
    concatTensorInfo->type = additionalInfo->type;
    concatTensorInfo->threadId = additionalInfo->threadId;
    concatTensorInfo->dataLen = additionalInfo->dataLen;
    concatTensorInfo->timeStamp = additionalInfo->timeStamp;
    auto tensorInfo = ReinterpretConvert<MsprofTensorInfo*, uint8_t*>(additionalInfo->data);
    concatTensorInfo->opName = tensorInfo->opName;
    concatTensorInfo->tensorNum = tensorInfo->tensorNum;
    for (uint32_t i = 0; i < tensorInfo->tensorNum; ++i) {
        concatTensorInfo->tensorData[i] = tensorInfo->tensorData[i];
    }
    return concatTensorInfo;
}

int TensorInfoParser::ProduceChunk()
{
    if (chunkProducer_->ReadChunk() != ANALYSIS_OK) {
        ERROR("Read Chunk failed.");
        return ANALYSIS_ERROR;
    }
    auto &chunkConsumer = chunkConsumers_[typeid(ConcatTensorInfo).hash_code()];
    auto currTensorInfo = ChunkGenerator::ToObj<MsprofAdditionalInfo>(chunkProducer_->Pop());
    while (currTensorInfo) {
        std::shared_ptr<ConcatTensorInfo> concatTensor = CreateConcatTensorInfo(currTensorInfo);
        if (!concatTensor) {
            ERROR("ConcatTensorInfo make_shared failed.");
            return ANALYSIS_ERROR;
        }
        while (currTensorInfo) {
            auto nextTensorInfo = ChunkGenerator::ToObj<MsprofAdditionalInfo>(chunkProducer_->Pop());
            currTensorInfo = nextTensorInfo;
            if (!nextTensorInfo) {
                break;
            }
            auto nextTensor = ReinterpretConvert<MsprofTensorInfo*, uint8_t*>(nextTensorInfo->data);
            if (nextTensor->opName != concatTensor->opName ||
                    nextTensorInfo->timeStamp != concatTensor->timeStamp ||
                    nextTensorInfo->threadId != concatTensor->threadId) {
                break;
            }
            // tensor info拼接
            if (concatTensor->tensorNum + nextTensor->tensorNum > TENSOR_DATA_MAX_NUM) {
                ERROR("Concat tensor info length > %", TENSOR_DATA_MAX_NUM);
                continue;
            }
            for (uint32_t i = 0; i < nextTensor->tensorNum; ++i) {
                concatTensor->tensorData[concatTensor->tensorNum] = nextTensor->tensorData[i];
                concatTensor->tensorNum += 1;
            }
        }
        chunkConsumer->Push(ChunkGenerator::ToChunk<ConcatTensorInfo>(concatTensor));
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis
