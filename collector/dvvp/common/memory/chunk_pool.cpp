/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#include "chunk_pool.h"
#include "securec.h"
#include "msprof_dlog.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace memory {
using namespace analysis::dvvp::common::utils;
Chunk::Chunk(size_t bufferSize)
    : buffer_(nullptr), bufferSize_(bufferSize), usedSize_(0)
{
}

Chunk::~Chunk()
{
    Uninit();
}

bool Chunk::Init()
{
    if (bufferSize_ == 0) {
        return true;
    }

    buffer_ = static_cast<UNSIGNED_CHAR_PTR>(malloc(bufferSize_));
    if (buffer_ == nullptr) {
        return false;
    }

    Clear();

    return true;
}

void Chunk::Uninit()
{
    if (buffer_ != nullptr) {
        free(buffer_);
        buffer_ = nullptr;
    }
    bufferSize_ = 0;
}

void Chunk::Clear()
{
    if (buffer_ != nullptr) {
        auto ret = memset_s(buffer_, bufferSize_ * sizeof(unsigned char), 0, bufferSize_ * sizeof(unsigned char));
        if (ret != EOK) {
            MSPROF_LOGE("memory chunk clear failed ret:%d", ret);
        }
    }
    usedSize_ = 0;
}

const unsigned char *Chunk::GetBuffer() const
{
    return buffer_;
}

size_t Chunk::GetUsedSize() const
{
    return usedSize_;
}

void Chunk::SetUsedSize(size_t size)
{
    usedSize_ = size;
}

size_t Chunk::GetFreeSize() const
{
    return bufferSize_ - usedSize_;
}

size_t Chunk::GetBufferSize() const
{
    return bufferSize_;
}

ChunkPool::ChunkPool(size_t poolSize, size_t chunkSize)
    : ResourcePool<Chunk, size_t>(poolSize, chunkSize)
{
}

ChunkPool::~ChunkPool()
{
}
}  // namespace memory
}  // namespace common
}  // namespace dvvp
}  // namespace analysis
