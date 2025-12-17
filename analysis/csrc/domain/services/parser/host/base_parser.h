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

#ifndef ANALYSIS_PARSER_BASE_PARSER_H
#define ANALYSIS_PARSER_BASE_PARSER_H

#include <vector>
#include <string>
#include <memory>

#include "analysis/csrc/infrastructure/dfx/error_code.h"
#include "analysis/csrc/infrastructure/dfx/log.h"
#include "analysis/csrc/domain/services/parser/host/chunk_generator.h"

namespace Analysis {
namespace Domain {
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
        if (ProduceData() != ANALYSIS_OK) {
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
}  // namespace Domain
}  // namespace Analysis

#endif // ANALYSIS_PARSER_BASE_PARSER_H
