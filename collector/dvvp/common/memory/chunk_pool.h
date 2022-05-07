/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_COMMON_MEMORY_CHUNK_POOL_H
#define ANALYSIS_DVVP_COMMON_MEMORY_CHUNK_POOL_H
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace memory {
class Chunk {
public:
    explicit Chunk(size_t bufferSize);
    virtual ~Chunk();

public:
    bool Init();
    void Uninit();

public:
    void Clear();
    const unsigned char *GetBuffer() const;

    size_t GetUsedSize() const;
    void SetUsedSize(size_t size);

    size_t GetFreeSize() const;
    size_t GetBufferSize() const;

private:
    Chunk &operator=(const Chunk &chunk);
    Chunk(const Chunk &chunk);

private:
    unsigned char *buffer_;
    size_t bufferSize_;
    size_t usedSize_;
};

template<class T, class ArgT>
class PoolBase {
public:
    explicit PoolBase(size_t poolSize, ArgT resArg)
        : poolSize_(poolSize), resArg_(resArg)
    {
    }
    virtual ~PoolBase() {}

    bool Init()
    {
        if (poolSize_ == 0) {
            return false;
        }

        for (size_t ii = 0; ii < poolSize_; ++ii) {
            auto res = AllocResource(resArg_);
            if (!res) {
                break;
            }

            freed_.push_back(res);
        }

        return (poolSize_ == freed_.size()) ? true : false;
    }

protected:
    size_t poolSize_;
    ArgT resArg_;
    std::list<SHARED_PTR_ALIA<T>> freed_;

private:
    SHARED_PTR_ALIA<T> AllocResource(ArgT arg)
    {
        SHARED_PTR_ALIA<T> res;

        do {
            MSVP_MAKE_SHARED1_BREAK(res, T, arg);
            if (!(res->Init())) {
                res.reset();
            }
        } while (0);

        return res;
    }
};

template<class T, class ArgT>
class ResourcePool : public PoolBase<T, ArgT> {
public:
    explicit ResourcePool(size_t poolSize, ArgT resArg)
        : PoolBase<T, ArgT>(poolSize, resArg)
    {
    }

    virtual ~ResourcePool()
    {
        Uninit();
    }

public:
    void Uninit()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        this->freed_.clear();
        used_.clear();
    }

    SHARED_PTR_ALIA<T> Alloc()
    {
        std::unique_lock<std::mutex> lk(mtx_);

        cvFree_.wait(lk, [this] { return !this->freed_.empty(); });

        auto res = this->freed_.front();
        this->freed_.pop_front();
        used_[(uintptr_t)res.get()] = res;

        return res;
    }

    SHARED_PTR_ALIA<T> TryAlloc()
    {
        std::lock_guard<std::mutex> lk(mtx_);

        if (this->freed_.empty()) {
            return nullptr;
        }

        auto res = this->freed_.front();
        this->freed_.pop_front();
        used_[(uintptr_t)res.get()] = res;

        return res;
    }

    void Release(SHARED_PTR_ALIA<T> res)
    {
        std::lock_guard<std::mutex> lk(mtx_);

        if (res) {
            auto iter = used_.find((uintptr_t)res.get());
            if (iter != used_.end()) {
                used_.erase(iter);
                this->freed_.push_back(res);

                cvFree_.notify_all();
            }
        }
    }

private:
    std::map<uintptr_t, SHARED_PTR_ALIA<T>> used_;
    std::condition_variable cvFree_;
    std::mutex mtx_;
};

class ChunkPool : public ResourcePool<Chunk, size_t> {
public:
    ChunkPool(size_t poolSize, size_t chunkSize);
    virtual ~ChunkPool();
};
}  // namespace memory
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
