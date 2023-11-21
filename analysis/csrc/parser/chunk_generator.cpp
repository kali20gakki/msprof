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

#include "chunk_generator.h"

#include "file.h"
#include "log.h"
#include "error_code.h"

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

std::shared_ptr<void> ChunkGenerator::Pop()
{
    if (remainSize_ == 0) {
        return nullptr;
    }
    std::shared_ptr<void> chunk;
    chunk.reset(new char[chunkSize_], std::default_delete<char[]>());
    ss_.read(std::static_pointer_cast<char>(chunk).get(), chunkSize_);
    --remainSize_;
    return chunk;
}

int ChunkGenerator::Push(const std::shared_ptr<void> &chunk)
{
    if (!chunk) {
        ERROR("Push chunk is null.");
        return ANALYSIS_ERROR;
    }
    if (chunkSize_ == 0) {
        ERROR("Chunk size is invalid");
        return ANALYSIS_ERROR;
    }
    ++remainSize_;
    ss_.write(std::static_pointer_cast<char>(chunk).get(), chunkSize_);
    return ANALYSIS_OK;
}

bool ChunkGenerator::FileEmpty() const
{
    return readFiles_.empty();
}

bool ChunkGenerator::ChunkEmpty() const
{
    return remainSize_ == 0;
}

size_t ChunkGenerator::Size() const
{
    return remainSize_;
}

std::streamsize ChunkGenerator::GetChunkSize() const
{
    return chunkSize_;
}
}  // namespace Parser
}  // namespace Analysis
