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
#include <functional>
#include "prof_common.h"
#include "bound_queue.h"
#include "memory/chunk_pool.h"
#include "prof_callback.h"
#include "prof_reporter.h"
#include "queue/ring_buffer.h"
#include "config/config.h"
#include "proto/msprofiler.pb.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp;
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

enum MsprofProfileDataType {
    MSPROF_DEFAULT_PROFILE_DATA_TYPE = 0,
    MSPROF_API_AND_EVENT_PROFILE_DATA_TYPE,
    MSPROF_COMPACT_PROFILE_DATA_TYPE,
    MSPROF_ADDITIONAL_PROFILE_DATA_TYPE
};

enum MsprofReporterId {
    AGING_API                 = 0,
    UNAGING_API               = 1,
    AGING_EVENT               = 2,
    UNAGING_EVENT             = 3,
    AGING_COMPACT_INFO        = 4,
    UNAGING_COMPACT_INFO      = 5,
    AGING_ADDITIONAL_INFO     = 6,
    UNAGING_ADDITIONAL_INFO   = 7
};

const std::vector<ModuleIdName> MSPROF_MODULE_REPORT_TABLE = {
    {AGING_API,                 "aging.api_event",       RING_BUFF_CAPACITY},
    {UNAGING_API,               "unaging.api_event",     RING_BUFF_CAPACITY},
    {AGING_EVENT,               "aging.event",           RING_BUFF_CAPACITY},
    {UNAGING_EVENT,             "unaging.event",         RING_BUFF_CAPACITY},
    {AGING_COMPACT_INFO,        "aging.compact",         RING_BUFF_CAPACITY},
    {UNAGING_COMPACT_INFO,      "unaging.compact",       RING_BUFF_CAPACITY},
    {AGING_ADDITIONAL_INFO,     "aging.additional",      RING_BUFF_CAPACITY},
    {UNAGING_ADDITIONAL_INFO,   "unaging.additional",    RING_BUFF_CAPACITY},
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
    virtual void DumpDynProfCachedMsg(const std::string &devId)
    {
        UNUSED(devId);
        MSPROF_LOGI("ReceiveData::DumpDynProfCachedMsg, devId is %s", devId.c_str());
    }
    virtual int SendData(SHARED_PTR_ALIA<ProfileFileChunk> fileChunk);
    int32_t DoReportData(const MsprofApi &dataChunk);
    int32_t DoReportData(const MsprofCompactInfo &dataChunk);
    int32_t DoReportData(const MsprofAdditionalInfo &dataChunk);

protected:
    virtual void StopReceiveData();
    virtual int Flush();
    virtual void TimedTask();
    virtual void DoReportRun();
    virtual int32_t Dump(std::vector<SHARED_PTR_ALIA<ProfileFileChunk>> &message);
    virtual void WriteDone();
    void RunDefaultProfileData(std::vector<SHARED_PTR_ALIA<ProfileFileChunk>>& fileChunks);
    int32_t DumpData(std::vector<ReporterDataChunk> &message, SHARED_PTR_ALIA<ProfileFileChunk> fileChunk);
    void SetBufferEmptyEvent();
    void WaitBufferEmptyEvent(uint64_t us);
    int Init(size_t capacity);
    int DoReport(CONST_REPORT_DATA_PTR rData);
    template<typename T>
    int32_t DumpData(std::vector<T> &message, SHARED_PTR_ALIA<ProfileFileChunk> fileChunk);
    template<typename T>
    void RunNoTagData(analysis::dvvp::common::queue::RingBuffer<T> &dataBuffer,
        std::vector<SHARED_PTR_ALIA<ProfileFileChunk>>& fileChunks);
    template<typename T>
    void RunTagData(analysis::dvvp::common::queue::RingBuffer<T> &dataBuffer,
        std::vector<SHARED_PTR_ALIA<ProfileFileChunk>>& fileChunks);
   
    void PrintTotalSize();
    void RunProfileData(std::vector<SHARED_PTR_ALIA<ProfileFileChunk>>& fileChunks);

protected:
    volatile bool started_;
    volatile bool stopped_;
    analysis::dvvp::common::queue::RingBuffer<ReporterDataChunk> dataChunkBuf_;
    std::string moduleName_;
    std::set<std::string> devIds_;
    int32_t profileDataType_;
    uint64_t totalCountFromRingBuff_;
    uint64_t totalDataLengthFromRingBuff_;
    unsigned long long timeStamp_;

private:
    int DoReportData(const ReporterDataChunk& dataChunk);
    bool IsNewDataStructModule(const std::string &module) const;

    void SetDataChunkTag(uint16_t level, uint32_t typeId, std::string &tag) const;

private:

    analysis::dvvp::common::queue::RingBuffer<MsprofApi> dataBufApi_;
    analysis::dvvp::common::queue::RingBuffer<MsprofCompactInfo> dataBufCompactInfo_;
    analysis::dvvp::common::queue::RingBuffer<MsprofAdditionalInfo> dataBufAdditionalInfo_;
    std::condition_variable cvBufferEmpty_;
    std::mutex cvBufferEmptyMtx_;
    std::atomic<uint64_t> totalPushCounter_;
    std::atomic<uint64_t> totalPushCounterSuccess_;
    std::atomic<uint64_t> totalDataLengthSuccess_;
    std::atomic<uint64_t> totalPushCounterFailed_;
    std::atomic<uint64_t> totalDataLengthFailed_;

    std::function<void(std::vector<SHARED_PTR_ALIA<ProfileFileChunk>>&)> runUploader_;
};
}}
#endif

