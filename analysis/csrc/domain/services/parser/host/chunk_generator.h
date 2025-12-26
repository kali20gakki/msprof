/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#ifndef ANALYSIS_PARSER_CHUNK_GENERATOR_H
#define ANALYSIS_PARSER_CHUNK_GENERATOR_H

#include <string>
#include <sstream>
#include <queue>

#include "analysis/csrc/infrastructure/utils/utils.h"

namespace Analysis {
namespace Domain {
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
}  // namespace Domain
}  // namespace Analysis

#endif // ANALYSIS_PARSER_CHUNK_GENERATOR_H
