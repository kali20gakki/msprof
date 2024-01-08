/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : chunk_generator.cpp
 * Description        : 数据chunk生成类
 * Author             : msprof team
 * Creation Date      : 2023/11/18
 * *****************************************************************************
 */

#include "analysis/csrc/parser/chunk_generator.h"

#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/utils/file.h"

namespace Analysis {
namespace Parser {
using namespace Analysis::Utils;

ChunkGenerator::ChunkGenerator(uint32_t chunkSize, const std::string &path, const std::vector<std::string> &filePrefix)
    : chunkSize_(chunkSize)
{
    auto files = File::GetOriginData(path, filePrefix, {"done", "complete"});
    std::move(
        files.begin(),
        files.end(),
        back_inserter(readFiles_)
    );
}

ChunkGenerator::~ChunkGenerator()
{
    readFiles_.clear();
    remainSize_ = 0;
    ss_.clear();
    ss_.str("");
}

int ChunkGenerator::ReadChunk()
{
    if (chunkSize_ == 0) {
        ERROR("Chunk size is invalid");
        return ANALYSIS_ERROR;
    }
    while (!readFiles_.empty()) {
        auto file = readFiles_.front();
        readFiles_.pop_front();
        FileReader fd(file, std::ios::in | std::ios::binary);
        if (fd.ReadBinary(ss_) != ANALYSIS_OK) {
            ERROR("The read chunk binary failed: %", file);
            return ANALYSIS_ERROR;
        }
        auto fileSize = File::Size(file);
        if (fileSize % chunkSize_ != 0) {
            ERROR("The read chunk file size is invalid: %", file);
            return ANALYSIS_ERROR;
        }
        remainSize_ += fileSize / chunkSize_;
    }
    return ANALYSIS_OK;
}

CHAR_PTR ChunkGenerator::Pop()
{
    if (remainSize_ == 0) {
        WARN("Nothing remain to Pop.");
        return nullptr;
    }
    auto chunk = new (std::nothrow) char[chunkSize_];
    if (!chunk) {
        ERROR("New chunk failed.");
        return nullptr;
    }
    ss_.read(chunk, chunkSize_);
    --remainSize_;
    return chunk;
}

bool ChunkGenerator::Empty() const
{
    return remainSize_ == 0;
}

size_t ChunkGenerator::Size() const
{
    return remainSize_;
}
}  // namespace Parser
}  // namespace Analysis
