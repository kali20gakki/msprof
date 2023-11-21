/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : base_parser.h
 * Description        : 数据解析对外接口
 * Author             : msprof team
 * Creation Date      : 2023/11/8
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_BASE_PARSER_H
#define ANALYSIS_PARSER_BASE_PARSER_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "chunk_generator.h"
#include "error_code.h"
#include "log.h"

namespace Analysis {
namespace Parser {
// 该类是数据解析的对外接口，提供接口GetData()，读取不同类型数据并返回
class BaseParser {
public:
    explicit BaseParser(const std::string &path) : path_(path) {}

    template<typename T>
    std::vector<std::shared_ptr<T>> GetData()
    {
        if (!chunkProducer_) {
            ERROR("The chunk producer is null");
            return {};
        }
        if (ProduceChunk() != ANALYSIS_OK) {
            ERROR("Producer chunk error.");
            return {};
        }
        auto iter = chunkConsumers_.find(typeid(T).hash_code());
        if (iter == chunkConsumers_.end() || !iter->second) {
            ERROR("The chunk consumer is null");
            return {};
        }
        auto chunkConsumer = iter->second;
        std::vector<std::shared_ptr<T>> data;
        data.reserve(chunkConsumer->Size());
        while (!chunkConsumer->ChunkEmpty()) {
            std::shared_ptr<void> chunk;
            if (ConsumeChunk(chunk, chunkConsumer) != ANALYSIS_OK) {
                ERROR("Consume chunk error.");
                return {};
            }
            data.emplace_back(ChunkGenerator::ToObj<T>(chunk));
        }
        return data;
    }

protected:
    virtual int ProduceChunk();
    virtual int ConsumeChunk(std::shared_ptr<void> &chunk, const std::shared_ptr<ChunkGenerator> &chunkConsumer);

protected:
    std::string path_;
    std::shared_ptr<ChunkGenerator> chunkProducer_;
    std::unordered_map<size_t, std::shared_ptr<ChunkGenerator>> chunkConsumers_;
};  // class BaseParser
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_BASE_PARSER_H
