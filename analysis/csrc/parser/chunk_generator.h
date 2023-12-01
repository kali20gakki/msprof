/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chunk_generator.h
 * Description        : 数据chunk生成类
 * Author             : msprof team
 * Creation Date      : 2023/11/8
 * *****************************************************************************
 */

#ifndef ANALYSIS_PARSER_CHUNK_GENERATOR_H
#define ANALYSIS_PARSER_CHUNK_GENERATOR_H

#include <string>
#include <sstream>
#include <queue>

#include "utils.h"

namespace Analysis {
namespace Parser {
// 该类是数据chunk生成类
// 主要包括以下特性：
// 1. 根据文件名的前后缀，获取要读取的文件路径
// 2. ReadChunk：从文件读取二进制数据，保存在std::stringstream对象中
// 3. Pop：从std::stringstream对象Pop出chunkSize_大小的二进制数据
class ChunkGenerator {
public:
    explicit ChunkGenerator(uint32_t chunkSize) : chunkSize_(chunkSize) {}
    ChunkGenerator(uint32_t chunkSize, const std::string &path, const std::vector<std::string> &filePrefix);
    virtual ~ChunkGenerator();

    int ReadChunk();
    Utils::CHAR_PTR Pop();
    bool Empty() const;
    size_t Size() const;

private:
    std::stringstream ss_;
    std::deque<std::string> readFiles_;
    std::streamsize chunkSize_ = 0;  // one data chunk bytes
    size_t remainSize_ = 0;  // remain chunk number in ss_
};  // class ChunkGenerator
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_CHUNK_GENERATOR_H
