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
#include <memory>
#include <queue>

namespace Analysis {
namespace Parser {
// 该类是数据chunk生成类
// 主要包括以下特性：
// 1. 根据正则表达式，获取要读取的文件路径
// 2. Push从文件读取二进制数据，保存在std::stringstream对象中
// 3. Pop从std::stringstream对象Pop出固定大小的二进制数据chunk
class ChunkGenerator {
public:
    explicit ChunkGenerator(uint32_t chunkSize) : chunkSize_(chunkSize) {}
    ChunkGenerator(uint32_t chunkSize, const std::string &path, const std::vector<std::string> &filePrefix);
    virtual ~ChunkGenerator();

    int ReadChunk();
    int Push(const std::shared_ptr<void> &chunk);
    std::shared_ptr<void> Pop();
    bool FileEmpty() const;
    bool ChunkEmpty() const;
    size_t Size() const;

    template<typename T>
    static std::shared_ptr<T> ToObj(std::shared_ptr<void> chunk)
    {
        return std::static_pointer_cast<T>(chunk);
    }

    template<typename T>
    static std::shared_ptr<void> ToChunk(std::shared_ptr<T> obj)
    {
        return std::static_pointer_cast<void>(obj);
    }

private:
    std::stringstream ss_;
    std::deque<std::string> readFiles_;
    size_t chunkSize_ = 0;  // one data chunk bytes
    size_t remainSize_ = 0;  // remain bytes in ss_
};  // class ChunkGenerator
}  // namespace Parser
}  // namespace Analysis

#endif // ANALYSIS_PARSER_CHUNK_GENERATOR_H
