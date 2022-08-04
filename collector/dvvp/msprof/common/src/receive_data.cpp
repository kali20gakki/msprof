/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: receive data
 * Author: dml
 * Create: 2018-06-13
 */
#include "receive_data.h"
#include "config/config.h"
#include "error_code.h"
#include "prof_params.h"
#include "msprof_dlog.h"
#include "proto/profiler.pb.h"
#include "queue/ring_buffer.h"
#include "utils.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
static const ReporterDataChunk DEFAULT_DATA_CHUNK = { { 0 }, 0, 0, 0, { 0 } };

ReceiveData::ReceiveData()
    : started_(false),
      stopped_(false),
      dataChunkBuf_(DEFAULT_DATA_CHUNK),
      totalPushCounter_(0),
      totalPushCounterSuccess_(0),
      totalDataLengthSuccess_(0),
      totalPushCounterFailed_(0),
      totalDataLengthFailed_(0),
      totalCountFromRingBuff_(0),
      totalDataLengthFromRingBuff_(0),
      timeStamp_(0) {}

ReceiveData::~ReceiveData()
{
    StopReceiveData();
}

/**
* @brief Run: read datas from ring buffer to build FileChunkReq,
*             the read data times is MAX_LOOP_TIMES
* @param [out] fileChunks: data from user to write to local file or send to remote host
*/
void ReceiveData::Run(std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks)
{
    std::map<std::string, std::vector<ReporterDataChunk>> dataMap; // data binding with tag
    uint64_t batchSizeMax = 0;
    for (; batchSizeMax < MSVP_BATCH_MAX_LEN;) {
        ReporterDataChunk data;
        bool isOK = dataChunkBuf_.TryPop(data);
        if (!isOK) {
            break;
        }
        totalCountFromRingBuff_++;
        totalDataLengthFromRingBuff_ += data.dataLen;
        batchSizeMax += data.dataLen;

        std::string tagWizSuffix = std::string(data.tag) + "." + std::to_string(data.deviceId);
        // classify data by tag.deviceId
        auto iter = dataMap.find(tagWizSuffix);
        if (iter == dataMap.end()) {
            std::vector<ReporterDataChunk> dataChunkForTag;
            dataChunkForTag.push_back(data);
            dataMap[tagWizSuffix] = dataChunkForTag;
        } else {
            iter->second.push_back(data);
        }
    }
    for (auto iter = dataMap.begin(); iter != dataMap.end(); ++iter) {
        if (iter->second.size() > 0) {
            SHARED_PTR_ALIA<FileChunkReq> fileChunk = nullptr;
            MSVP_MAKE_SHARED0_BREAK(fileChunk, FileChunkReq);
            MSPROF_LOGD("Dump data, module:%s, key:%s, data.size:%llu, data[0].len:%llu",
                moduleName_.c_str(), iter->first.c_str(), iter->second.size(), iter->second[0].dataLen);
            if (DumpData(iter->second, fileChunk) == PROFILING_SUCCESS) {
                fileChunks.push_back(fileChunk); // insert the data into the new vector
            } else {
                MSPROF_LOGE("Dump Received Data failed");
            }
        }
    }
}

void ReceiveData::StopReceiveData()
{
    stopped_ = true;
    MSPROF_LOGI("stop this reporter");
}

void ReceiveData::WriteDone()
{
}

void ReceiveData::TimedTask()
{
}

int ReceiveData::SendData(SHARED_PTR_ALIA<FileChunkReq> fileChunk /* = nullptr */)
{
    UNUSED(fileChunk);
    MSPROF_LOGI("ReceiveData::SendData");
    return PROFILING_SUCCESS;
}

void ReceiveData::SetBufferEmptyEvent()
{
    std::lock_guard<std::mutex> lk(cvBufferEmptyMtx_);
    cvBufferEmpty_.notify_all();
}

void ReceiveData::WaitBufferEmptyEvent(uint64_t us)
{
    std::unique_lock<std::mutex> lk(cvBufferEmptyMtx_);
    auto res = cvBufferEmpty_.wait_for(lk, std::chrono::microseconds(us));
    if (res == std::cv_status::timeout) {
        MSPROF_LOGW("Wait buf empty timeout, moduleName:%s", moduleName_.c_str());
    }
}

/**
* @brief Flush: wait the buffer is empty
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int ReceiveData::Flush()
{
    static const int WAIT_BUFF_EMPTY_TIMEOUT_US = 5000000;
    WaitBufferEmptyEvent(WAIT_BUFF_EMPTY_TIMEOUT_US);
    return PROFILING_SUCCESS;
}

void ReceiveData::PrintTotalSize()
{
    uint64_t totalPush = totalPushCounter_.load(std::memory_order_relaxed);
    uint64_t totalSuccessPush = totalPushCounterSuccess_.load(std::memory_order_relaxed);
    uint64_t totalFailedPush = totalPushCounterFailed_.load(std::memory_order_relaxed);
    uint64_t totalDataLengthSuccess = totalDataLengthSuccess_.load(std::memory_order_relaxed);
    uint64_t totalDataLengthFailed = totalDataLengthFailed_.load(std::memory_order_relaxed);
    MSPROF_EVENT("total_size_report For DoReport, module:%s, totalPushCounter_:%llu, totalPushCounterSuccess_:%llu,"
                 " totalPushCounterFailed_:%llu, totalDataLengthSuccess_:%llu, totalDataLengthFailed_:%llu",
                 moduleName_.c_str(), totalPush, totalSuccessPush, totalFailedPush, totalDataLengthSuccess,
                 totalDataLengthFailed);
    MSPROF_EVENT("total_size_report For DoReportRun, module:%s, totalCountFromRingBuff_:%llu,"
                 " totalDataLengthFromRingBuff_:%llu",
                 moduleName_.c_str(), totalCountFromRingBuff_, totalDataLengthFromRingBuff_);
}

int ReceiveData::Init(size_t capacity)
{
    static const std::string RECEIVE_DATA_BUF_NAME = "ReceiveDataRingBuffer";
    dataChunkBuf_.Init(capacity);
    dataChunkBuf_.SetName(RECEIVE_DATA_BUF_NAME);
    return PROFILING_SUCCESS;
}

int ReceiveData::DoReport(CONST_REPORT_DATA_PTR rData)
{
    int ret = PROFILING_FAILED;
    ReporterDataChunk dataChunk;
    auto retCheck = memset_s(&dataChunk, sizeof(dataChunk), 0, sizeof(dataChunk));
    FUNRET_CHECK_FAIL_RET_VALUE(retCheck, EOK, PROFILING_FAILED);

    uint64_t startRawTime = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
    do {
        if (!started_ || rData == nullptr) {
            MSPROF_LOGE("report failed! started %llu", started_);
            break;
        }
        if (rData->data == nullptr || rData->dataLen <= 0) {
            MSPROF_LOGE("report failed! dataLen %llu", rData->dataLen);
            break;
        }
        if (rData->dataLen > RECEIVE_CHUNK_SIZE) {
            MSPROF_LOGE("module:%s, tag(%s) dataLen:%d exceeds %d", moduleName_.c_str(),
                        rData->tag, rData->dataLen, RECEIVE_CHUNK_SIZE);
            break;
        }
        if (moduleName_ == "runtime" && rData->deviceId >= 64) {  // runtime module, deviceid 64, return
            MSPROF_LOGI("module:%s, invalid device id:%d", moduleName_.c_str(), rData->deviceId);
            return PROFILING_SUCCESS;
        }
        dataChunk.deviceId = rData->deviceId;
        dataChunk.reportTime = startRawTime;
        dataChunk.dataLen = rData->dataLen;
        errno_t err = memcpy_s(dataChunk.tag, MSPROF_ENGINE_MAX_TAG_LEN + 1, rData->tag, MSPROF_ENGINE_MAX_TAG_LEN);
        if (err != EOK) {
            MSPROF_LOGE("memcpy tag failed, err:%d, deviceID:%d, tag:%s, dataLen:%llu",
                        static_cast<int>(err), dataChunk.deviceId, rData->tag, dataChunk.dataLen);
            break;
        }
        err = memcpy_s(dataChunk.data, RECEIVE_CHUNK_SIZE, rData->data, rData->dataLen);
        if (err != EOK) {
            MSPROF_LOGE("memcpy data failed, err:%d, deviceID:%d, tag:%s, dataLen:%llu",
                        static_cast<int>(err), dataChunk.deviceId, rData->tag, dataChunk.dataLen);
            break;
        }
        if (DoReportData(dataChunk) != PROFILING_SUCCESS) {
            break;
        }
        ret = PROFILING_SUCCESS;
    } while (0);

    return ret;
}

void ReceiveData::DoReportRun()
{
    std::vector<SHARED_PTR_ALIA<FileChunkReq> > fileChunks;
    unsigned long sleepIntevalNs = 500000000; // 500000000 : 500ms
    timeStamp_ = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();

    for (;;) {
        fileChunks.clear();
        Run(fileChunks);
        size_t size = fileChunks.size();
        if (size == 0) {
            SetBufferEmptyEvent();
            if (stopped_) {
                WriteDone();
                MSPROF_LOGI("Exit the Run thread");
                break;
            }
            analysis::dvvp::common::utils::Utils::UsleepInterupt(SLEEP_INTEVAL_US);
            unsigned long long curTimeStamp = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
            if ((curTimeStamp - timeStamp_) >= sleepIntevalNs || (timeStamp_ == 0)) {
                TimedTask();
                timeStamp_ = curTimeStamp;
            }
            continue;
        }
        if (Dump(fileChunks) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Dump Received Data failed");
        }
    }
}

/**
* @brief DoReportData: get the data from user and save the data to ring buffer
* @param [in] dataChunk: the data from user
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int ReceiveData::DoReportData(const ReporterDataChunk& dataChunk)
{
    int retu = PROFILING_FAILED;
    totalPushCounter_++;
    bool ret = dataChunkBuf_.TryPush(dataChunk);
    if (!ret) {
        totalPushCounterFailed_++;
        totalDataLengthFailed_ += dataChunk.dataLen;
        uint64_t totalLengthFailed = totalDataLengthFailed_.load(std::memory_order_relaxed);
        uint64_t totalPushFailed = totalPushCounterFailed_.load(std::memory_order_relaxed);
        size_t buffUsedSize = dataChunkBuf_.GetUsedSize();
        MSPROF_LOGE("try push ring buff failed, deviceID:%d, module:%s, tag:%s, dataLen:%llu,"
            " totalPushCounterFailed_:%llu, totalDataLengthFailed_:%llu, buffUsedSize:%llu",
            dataChunk.deviceId, moduleName_.c_str(), dataChunk.tag, dataChunk.dataLen,
            totalPushFailed, totalLengthFailed, buffUsedSize);
        retu = PROFILING_FAILED;
    } else {
        totalPushCounterSuccess_++;
        totalDataLengthSuccess_ += dataChunk.dataLen;
        retu = PROFILING_SUCCESS;
    }
    return retu;
}

int ReceiveData::Dump(std::vector<SHARED_PTR_ALIA<FileChunkReq> > &message)
{
    UNUSED(message);
    MSPROF_LOGD("message size is : %u", message.size());
    return PROFILING_SUCCESS;
}

/**
* @brief DumpData: deal with the data from user to build FileChunkReq struct for store or send
* @param [in] message: data saved in the ring buffer
* @param [out] fileChunk: data build from message for store or send
* @return : success return PROFILING_SUCCESS, failed return PROFILING_FAILED
*/
int ReceiveData::DumpData(std::vector<ReporterDataChunk> &message, SHARED_PTR_ALIA<FileChunkReq> fileChunk)
{
    if (fileChunk == nullptr) {
        MSPROF_LOGE("fileChunk or dataPool is nullptr");
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx = nullptr;
    MSVP_MAKE_SHARED0_RET(jobCtx, analysis::dvvp::message::JobContext, PROFILING_FAILED);
    size_t chunkLen = 0;
    std::string chunk = "";
    bool isFirstMessage = true;
    for (size_t i = 0; i < message.size(); i++) {
        size_t messageLen = static_cast<size_t>(message[i].dataLen);
        CHAR_PTR dataPtr = reinterpret_cast<CHAR_PTR>(&message[i].data[0]);
        if (dataPtr == nullptr) {
            return PROFILING_FAILED;
        }
        if (isFirstMessage) { // deal with the data only need to init once
            jobCtx->dev_id = std::to_string(message[i].deviceId);
            jobCtx->tag = std::string(message[i].tag, strlen(message[i].tag));
            fileChunk->set_filename(moduleName_);
            fileChunk->set_offset(-1);
            chunkLen = messageLen;
            chunk = std::string(dataPtr, chunkLen);
            fileChunk->set_islastchunk(false);
            fileChunk->set_needack(false);
            fileChunk->set_tag(std::string(message[i].tag, strlen(message[i].tag)));
            fileChunk->set_tagsuffix(jobCtx->dev_id);
            fileChunk->set_chunkstarttime(message[i].reportTime);
            fileChunk->set_chunkendtime(message[i].reportTime);
            isFirstMessage = false;
            devIds_.insert(jobCtx->dev_id);
        } else { // deal with the data need to update according every message
            chunk.insert(chunkLen, std::string(dataPtr, messageLen));
            chunkLen += messageLen;
            fileChunk->set_chunkendtime(message[i].reportTime);
        }
    }
    jobCtx->module = moduleName_;
    jobCtx->chunkStartTime = fileChunk->chunkstarttime();
    jobCtx->chunkEndTime = fileChunk->chunkendtime();
    jobCtx->dataModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF;
    fileChunk->mutable_hdr()->set_job_ctx(jobCtx->ToString());
    fileChunk->set_chunk(chunk);
    fileChunk->set_chunksizeinbytes(chunkLen);
    return PROFILING_SUCCESS;
}
}}
