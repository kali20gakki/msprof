/**
* @file receive_data.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef MSPROF_ENGINE_RECEIVE_DATA_H
#define MSPROF_ENGINE_RECEIVE_DATA_H

#include <atomic>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <thread>
#include "bound_queue.h"
#include "codec.h"
#include "memory/chunk_pool.h"
#include "prof_callback.h"
#include "prof_reporter.h"
#include "queue/ring_buffer.h"
#include "config/config.h"
#include "proto/profiler.pb.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::proto;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::memory;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::queue;
struct ModuleIdName {
    uint32_t id;
    std::string name;
    size_t ringBufSize;
};
const std::vector<ModuleIdName> MSPROF_MODULE_ID_NAME_MAP = {
    {MSPROF_MODULE_DATA_PREPROCESS, "DATA_PREPROCESS", RING_BUFF_CAPACITY},
    {MSPROF_MODULE_HCCL, "HCCL", RING_BUFF_CAPACITY},
    {MSPROF_MODULE_ACL, "AclModule", RING_BUFF_CAPACITY},
    {MSPROF_MODULE_FRAMEWORK, "Framework", GE_RING_BUFF_CAPACITY},
    {MSPROF_MODULE_RUNTIME, "runtime", RING_BUFF_CAPACITY},
    {MSPROF_MODULE_MSPROF, "Msprof", MSPROF_RING_BUFF_CAPACITY},
};

struct ReporterDataChunk {
    int32_t deviceId;
    uint32_t dataLen;
    uint64_t reportTime;
    ReporterDataChunkTag tag;
    ReporterDataChunkPayload data;
};

class ReceiveData {
public:
    ReceiveData();
    virtual ~ReceiveData();

public:
    virtual void DumpModelLoadData(const std::string &devId)
    {
        UNUSED(devId);
        MSPROF_LOGI("ReceiveData::DumpModelLoadData, devId is %s", devId.c_str());
    }
    virtual int SendData(SHARED_PTR_ALIA<FileChunkReq> fileChunk);
    virtual void DumpDynProfCachedMsg(const std::string &devId)
    {
        UNUSED(devId);
        MSPROF_LOGI("ReceiveData::DumpDynProfCachedMsg, devId is %s", devId.c_str());
    }

protected:
    virtual void StopReceiveData();
    virtual int Flush();
    virtual void TimedTask();
    void Run(std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks);
    int DumpData(std::vector<ReporterDataChunk> &message, SHARED_PTR_ALIA<FileChunkReq> fileChunk);
    void SetBufferEmptyEvent();
    void WaitBufferEmptyEvent(uint64_t us);
    int Init(size_t capacity);
    void DoReportRun();
    int DoReport(CONST_REPORT_DATA_PTR rData);
    virtual int Dump(std::vector<SHARED_PTR_ALIA<FileChunkReq>> &message);
    virtual void WriteDone();
    void PrintTotalSize();

protected:
    volatile bool started_;
    volatile bool stopped_;
    std::string moduleName_;
    std::set<std::string> devIds_;

private:
    int DoReportData(CONST_REPORT_DATA_PTR rData);

private:
    analysis::dvvp::common::queue::RingBuffer<ReporterDataChunk> dataChunkBuf_;
    std::condition_variable cvBufferEmpty_;
    std::mutex cvBufferEmptyMtx_;
    std::atomic<uint64_t> totalPushCounter_;
    std::atomic<uint64_t> totalPushCounterSuccess_;
    std::atomic<uint64_t> totalDataLengthSuccess_;
    std::atomic<uint64_t> totalPushCounterFailed_;
    std::atomic<uint64_t> totalDataLengthFailed_;
    uint64_t totalCountFromRingBuff_;
    uint64_t totalDataLengthFromRingBuff_;
    unsigned long long timeStamp_;
};
}}
#endif

