/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : api_event_parser.cpp
 * Description        : api event数据解析
 * Author             : msprof team
 * Creation Date      : 2023/11/9
 * *****************************************************************************
 */

#include "api_event_parser.h"

#include "chunk_generator.h"
#include "prof_common.h"
#include "log.h"
#include "utils.h"

namespace Analysis {
namespace Parser {
namespace Host {
namespace Cann {
using namespace Analysis::Utils;

ApiEventParser::ApiEventParser(const std::string &path) : BaseParser(path)
{
    chunkProducer_ = MakeShared<ChunkGenerator>(sizeof(MsprofApi), path_, filePrefix_);
    chunkConsumers_ = {
        {typeid(MsprofApi).hash_code(), MakeShared<ChunkGenerator>(sizeof(MsprofApi))},
        {typeid(MsprofEvent).hash_code(), MakeShared<ChunkGenerator>(sizeof(MsprofEvent))},
    };
}

int ApiEventParser::ProduceChunk()
{
    if (chunkProducer_->ReadChunk() != ANALYSIS_OK) {
        ERROR("Read Chunk failed.");
        return ANALYSIS_ERROR;
    }
    while (!chunkProducer_->ChunkEmpty()) {
        auto chunk = ChunkGenerator::ToObj<MsprofEvent>(chunkProducer_->Pop());
        if (!chunk) {
            ERROR("Chunk is null.");
            continue;
        }
        if (chunk->reserve == MSPROF_EVENT_FLAG) {
            chunkConsumers_[typeid(MsprofEvent).hash_code()]->Push(ChunkGenerator::ToChunk<MsprofEvent>(chunk));
        } else {
            chunkConsumers_[typeid(MsprofApi).hash_code()]->Push(ChunkGenerator::ToChunk<MsprofEvent>(chunk));
        }
    }
    return ANALYSIS_OK;
}

int ApiEventParser::ConsumeChunk(std::shared_ptr<void> &chunk, const std::shared_ptr<ChunkGenerator> &chunkConsumer)
{
    chunk = chunkConsumer->Pop();
    if (!chunk) {
        ERROR("chunk is null.");
        return ANALYSIS_ERROR;
    }
    auto data = ChunkGenerator::ToObj<MsprofApi>(chunk);
    if (data->magicNumber != MSPROF_DATA_HEAD_MAGIC_NUM) {
        ERROR("Check 5A5A failed in place &.", chunkConsumer->Size());
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}
}  // namespace Cann
}  // namespace Host
}  // namespace Parser
}  // namespace Analysis
