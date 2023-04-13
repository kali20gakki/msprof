/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: lock-free ring buffer
 * Author: hufengwei
 * Create: 2019-10-28
 */
#ifndef ANALYSIS_DVVP_COMMON_QUEUE_RING_BUFFER_H
#define ANALYSIS_DVVP_COMMON_QUEUE_RING_BUFFER_H

#include <atomic>
#include <cstdio>
#include <thread>
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <vector>
#include <queue>
#include "config/config.h"
#include "msprof_dlog.h"
#include "prof_callback.h"
#include "utils/utils.h"
namespace analysis {
namespace dvvp {
namespace common {
namespace queue {
using namespace analysis::dvvp::common::config;
static const size_t RING_BUFFER_DEFAULT_MAX_PUSH_CYCLES = 1024;
enum class DataStatus {
    DATA_STATUS_NOT_READY = 0,
    DATA_STATUS_READY = 1
};
struct ReporterDataChunkTag {
    char tag[MSPROF_ENGINE_MAX_TAG_LEN + 1];
};
struct ReporterDataChunkPayload {
    uint8_t data[RECEIVE_CHUNK_SIZE];
};
using CONST_REPORT_DATA_PTR = const ReporterData *;
using CONST_REPORT_CHUNK_TAG_PTR = const ReporterDataChunkTag *;
// NOTE:
// The ring buffer is only for multiple producers and single consumer model
// The capacity must be a power of two
template <class T>
class RingBuffer {
public:
    explicit RingBuffer(const T& initialVal, size_t maxCycles = RING_BUFFER_DEFAULT_MAX_PUSH_CYCLES)
        : capacity_(0),
          initialVal_(initialVal),
          maxCycles_(maxCycles),
          mask_(0),
          readIndex_(0),
          writeIndex_(0),
          idleWriteIndex_(0),
          isQuit_(false),
          isInited_(false),
          useExtdDataQueue_(false)
    {
        static const std::string RING_BUFFER_DEFAULT_NAME = "RingBuffer";
        name_ = RING_BUFFER_DEFAULT_NAME;
    }

    virtual ~RingBuffer()
    {
        UnInit();
    }

public:
    void Init(size_t capacity)
    {
        capacity_ = capacity;
        mask_ = capacity - 1;
        std::vector<T> data(capacity_, initialVal_);
        dataQueue_.swap(data);
        std::vector<uint64_t> dataAail(capacity_, static_cast<uint64_t>(DataStatus::DATA_STATUS_NOT_READY));
        dataAvails_.swap(dataAail);
        isInited_ = true;
        std::queue<T> empty;
        std::swap(extdDataQueue_, empty);
    }

    void UnInit()
    {
        if (isInited_) {
            isInited_ = false;
            isQuit_ = true;
            dataQueue_.clear();
            dataAvails_.clear();
        }
    }

    void SetName(const std::string& name)
    {
        name_ = name;
    }

    void SetQuit()
    {
        isQuit_ = true;
    }

    bool TryPush(CONST_REPORT_DATA_PTR data)
    {
        bool useExtdDataQueue = useExtdDataQueue_.load(std::memory_order_relaxed);
        if (useExtdDataQueue && isInited_ && !isQuit_) {
            return TryPushExtdDataQueue(data);
        }
        
        size_t currReadCusor = 0;
        size_t currWriteCusor = 0;
        size_t nextWriteCusor = 0;
        size_t cycles = 0;

        do {
            if (!isInited_ || isQuit_) {
                return false;
            }

            cycles++;
            if (cycles >= maxCycles_) {
                size_t size = GetUsedSize();
                MSPROF_LOGW("QueueName: %s, QueueCapacity:%llu, QueueSize:%llu",
                            name_.c_str(), capacity_, size);
                return false;
            }

            currReadCusor = readIndex_.load(std::memory_order_relaxed);
            currWriteCusor = idleWriteIndex_.load(std::memory_order_relaxed);
            nextWriteCusor = currWriteCusor + 1;
            if ((nextWriteCusor & mask_) == (currReadCusor & mask_)) {
                MSPROF_LOGW("IsFull, QueueName:%s, QueueCapacity:%llu, Read:%llu, Write:%lld, Used:%llu",
                            name_.c_str(), capacity_, currReadCusor, currWriteCusor, GetUsedSize());
                useExtdDataQueue_.store(true);
                return TryPushExtdDataQueue(data);
            }
        } while (!idleWriteIndex_.compare_exchange_weak(currWriteCusor, nextWriteCusor));

        size_t index = currWriteCusor & mask_;
        dataQueue_[index].deviceId = data->deviceId;
        dataQueue_[index].reportTime = 0;
        dataQueue_[index].dataLen = data->dataLen;
        dataQueue_[index].tag = *(reinterpret_cast<CONST_REPORT_CHUNK_TAG_PTR>(data->tag));
        errno_t err = memcpy_s(dataQueue_[index].data.data, RECEIVE_CHUNK_SIZE, data->data, data->dataLen);
        if (err != EOK) {
            MSPROF_LOGE("memcpy data failed, err:%d, deviceID:%d, tag:%s, dataLen:%llu",
                        static_cast<int>(err), dataQueue_[index].deviceId, data->tag, dataQueue_[index].dataLen);
            return false;
        }
        writeIndex_++;
        dataAvails_[index] = static_cast<uint64_t>(DataStatus::DATA_STATUS_READY);

        return true;
    }

    bool TryPop(T& data)
    {
        if (!isInited_) {
            return false;
        }

        size_t currReadCusor = readIndex_.load(std::memory_order_relaxed);
        size_t currWriteCusor = writeIndex_.load(std::memory_order_relaxed);
        if ((currReadCusor & mask_) == (currWriteCusor & mask_)) {
            bool useExtdDataQueue = useExtdDataQueue_.load(std::memory_order_relaxed);
            if (useExtdDataQueue && !isQuit_) {
                return TryPopExtdDataQueue(data);
            }
            return false;
        }

        size_t index = currReadCusor & mask_;
        if (dataAvails_[index]) {
            data = dataQueue_[index];
            dataAvails_[index] = static_cast<uint64_t>(DataStatus::DATA_STATUS_NOT_READY);
            readIndex_++;
            return true;
        }

        return false;
    }

    size_t GetUsedSize()
    {
        size_t readIndex = readIndex_.load(std::memory_order_relaxed);
        size_t writeIndex = writeIndex_.load(std::memory_order_relaxed);
        if (readIndex > writeIndex) {
            return capacity_ - ((readIndex & mask_) - (writeIndex & mask_)) + GetExtdDataQueueSize();
        }

        return writeIndex - readIndex + GetExtdDataQueueSize();
    }

private:
    bool TryPushExtdDataQueue(CONST_REPORT_DATA_PTR data)
    {
        T dataChunk;
        dataChunk.deviceId = data->deviceId;
        dataChunk.reportTime = 0;
        dataChunk.dataLen = data->dataLen;
        dataChunk.tag = *(reinterpret_cast<CONST_REPORT_CHUNK_TAG_PTR>(data->tag));
        errno_t err = memcpy_s(dataChunk.data.data, RECEIVE_CHUNK_SIZE, data->data, data->dataLen);
        if (err != EOK) {
            MSPROF_LOGE("memcpy data failed, err:%d, deviceID:%d, tag:%s, dataLen:%llu",
                        static_cast<int>(err), dataChunk.deviceId, data->tag, dataChunk.dataLen);
            return false;
        }
        std::lock_guard<std::mutex> lk(mtx_);
        extdDataQueue_.push(dataChunk);
        return true;
    }
 
    bool TryPopExtdDataQueue(T& data)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (extdDataQueue_.empty()) {
            return false;
        }
        data = extdDataQueue_.front();
        extdDataQueue_.pop();
        return true;
    }

    size_t GetExtdDataQueueSize()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return extdDataQueue_.size();
    }

private:
    size_t capacity_;
    T initialVal_;
    size_t maxCycles_;
    size_t mask_;
    std::atomic<size_t> readIndex_;
    std::atomic<size_t> writeIndex_;
    std::atomic<size_t> idleWriteIndex_;
    volatile bool isQuit_;
    bool isInited_;
    std::string name_;
    std::vector<T> dataQueue_;
    std::vector<uint64_t> dataAvails_;
    std::atomic<bool> useExtdDataQueue_;
    std::queue<T> extdDataQueue_;
    std::mutex mtx_;
};
}  // namespace queue
}  // namespace common
}  // namespace dvvp
}  // namespace analysis

#endif
