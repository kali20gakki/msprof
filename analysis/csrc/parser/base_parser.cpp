/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : base_parser.cpp
 * Description        : 数据解析对外接口
 * Author             : msprof team
 * Creation Date      : 2023/11/18
 * *****************************************************************************
 */

#include "base_parser.h"

#include "error_code.h"
#include "log.h"

namespace Analysis {
namespace Parser {
int BaseParser::ProduceChunk()
{
    if (chunkProducer_->ReadChunk() != ANALYSIS_OK) {
        ERROR("Read Chunk failed.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}

int BaseParser::ConsumeChunk(std::shared_ptr<void> &chunk, const std::shared_ptr<ChunkGenerator> &chunkConsumer)
{
    chunk = chunkConsumer->Pop();
    if (!chunk) {
        ERROR("chunk is null.");
        return ANALYSIS_ERROR;
    }
    return ANALYSIS_OK;
}
}  // namespace Parser
}  // namespace Analysis
