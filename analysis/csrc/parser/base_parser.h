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

#include "chunk_generator.h"
#include "error_code.h"
#include "log.h"

namespace Analysis {
namespace Parser {
// 该类是数据解析的对外接口，提供接口ParseData()，解析不同类型数据并返回
// ParseData获取的数据是裸指针，需要接口调用者释放内存
template<typename ParserType>
class BaseParser {
public:
    explicit BaseParser(std::string path, std::string parserName)
        : path_(std::move(path)), parserName_(std::move(parserName)) {}

    template<typename T> std::vector<std::shared_ptr<T>> ParseData()
    {
        if (!chunkProducer_) {
            ERROR("%: The chunk producer is null.", parserName_);
            return {};
        }
        if (chunkProducer_->ReadChunk() != ANALYSIS_OK) {
            ERROR("%: Read Chunk failed.", parserName_);
            return {};
        }
        if (!chunkProducer_->Empty() && ProduceData() != ANALYSIS_OK) {
            ERROR("%: Format data failed.", parserName_);
            return {};
        }
        return static_cast<ParserType*>(this)->template GetData<T>();
    }

protected:
    virtual int ProduceData() = 0;

protected:
    std::string path_;
    std::string parserName_;
    std::shared_ptr<ChunkGenerator> chunkProducer_;
};  // class BaseParser
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_BASE_PARSER_H
