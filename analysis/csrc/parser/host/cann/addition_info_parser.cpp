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

#include "analysis/csrc/parser/host/cann/addition_info_parser.h"

#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/parser/chunk_generator.h"
#include "analysis/csrc/utils/utils.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
using namespace Analysis::Utils;
namespace {
std::shared_ptr<ConcatTensorInfo> CreateConcatTensorInfo(MsprofAdditionalInfo *additionalInfo)
{
    if (!additionalInfo) {
        ERROR("Additional info is null.");
        return nullptr;
    }
    auto concatTensorInfo = MakeShared<ConcatTensorInfo>();
    if (!concatTensorInfo) {
        ERROR("New concat tensor failed.");
        return nullptr;
    }
    concatTensorInfo->level = additionalInfo->level;
    concatTensorInfo->type = additionalInfo->type;
    concatTensorInfo->threadId = additionalInfo->threadId;
    concatTensorInfo->dataLen = additionalInfo->dataLen;
    concatTensorInfo->timeStamp = additionalInfo->timeStamp;
    auto tensorInfo = ReinterpretConvert<MsprofTensorInfo*>(additionalInfo->data);
    concatTensorInfo->opName = tensorInfo->opName;
    concatTensorInfo->tensorNum = tensorInfo->tensorNum;
    for (uint32_t i = 0; i < tensorInfo->tensorNum; ++i) {
        concatTensorInfo->tensorData[i] = tensorInfo->tensorData[i];
    }
    return concatTensorInfo;
}
}  // namespace

void AdditionInfoParser::Init(const std::vector<std::string> &filePrefix)
{
    chunkProducer_ = MakeShared<ChunkGenerator>(sizeof(MsprofAdditionalInfo), path_, filePrefix);
}

template<>
std::vector<std::shared_ptr<MsprofAdditionalInfo>> AdditionInfoParser::GetData()
{
    return additionalData_;
}

template<>
std::vector<std::shared_ptr<ConcatTensorInfo>> AdditionInfoParser::GetData()
{
    return concatTensorData_;
}

int AdditionInfoParser::ProduceData()
{
    if (!Reserve(additionalData_, chunkProducer_->Size())) {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->Empty()) {
        auto additionalInfo = ReinterpretConvert<MsprofAdditionalInfo*>(chunkProducer_->Pop());
        if (!additionalInfo) {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (additionalInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete additionalInfo;
            continue;
        }
        additionalData_.emplace_back(std::shared_ptr<MsprofAdditionalInfo>(additionalInfo));
    }
    return ANALYSIS_OK;
}

int TensorInfoParser::ProduceData()
{
    if (!Reserve(concatTensorData_, chunkProducer_->Size())) {
        ERROR("%: Reserve data failed", parserName_);
        return ANALYSIS_ERROR;
    }
    std::shared_ptr<ConcatTensorInfo> concatTensor = nullptr;
    while (!chunkProducer_->Empty()) {
        auto currTensorInfo = ReinterpretConvert<MsprofAdditionalInfo*>(chunkProducer_->Pop());
        if (!currTensorInfo) {
            ERROR("%: Pop chunk failed.", parserName_);
            return ANALYSIS_ERROR;
        }
        if (currTensorInfo->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
            ERROR("%: The last %th data check failed.", parserName_, chunkProducer_->Size());
            delete currTensorInfo;
            continue;
        }
        auto currTensor = ReinterpretConvert<MsprofTensorInfo*>(currTensorInfo->data);
        if (!concatTensor ||
                currTensor->opName != concatTensor->opName ||
                currTensorInfo->timeStamp != concatTensor->timeStamp ||
                currTensorInfo->threadId != concatTensor->threadId) {
            if (concatTensor) {
                concatTensorData_.emplace_back(concatTensor);
            }
            concatTensor = CreateConcatTensorInfo(currTensorInfo);
            delete currTensorInfo;
            if (!concatTensor) {
                ERROR("%: Create concat tensor failed.");
                return ANALYSIS_ERROR;
            }
            continue;
        }
        // tensor info拼接
        if (concatTensor->tensorNum + currTensor->tensorNum > TENSOR_DATA_MAX_NUM) {
            ERROR("Concat tensor info length > %", TENSOR_DATA_MAX_NUM);
            delete currTensorInfo;
            continue;
        }
        for (uint32_t i = 0; i < currTensor->tensorNum; ++i) {
            concatTensor->tensorData[concatTensor->tensorNum] = currTensor->tensorData[i];
            concatTensor->tensorNum += 1;
        }
        delete currTensorInfo;
    }
    if (concatTensor) {
        concatTensorData_.emplace_back(concatTensor);
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis
