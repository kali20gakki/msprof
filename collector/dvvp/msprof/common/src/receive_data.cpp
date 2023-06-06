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
#include "proto/msprofiler.pb.h"
#include "queue/ring_buffer.h"
#include "utils.h"
#include "msprof_reporter_mgr.h"

namespace Msprof {
namespace Engine {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
static const ReporterDataChunk DEFAULT_DATA_CHUNK = {0, 0, 0, {0}, {0}};

ReceiveData::ReceiveData()
    : started_(false),
      stopped_(false),
      dataChunkBuf_(DEFAULT_DATA_CHUNK),
      dataBufApi_(MsprofApi()),
      dataBufEvent_(MsprofEvent()),
      dataBufCompactInfo_(MsprofCompactInfo()),
      dataBufAdditionalInfo_(MsprofAdditionalInfo()),
      profileDataType_(MSPROF_DEFAULT_PROFILE_DATA_TYPE),
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

        std::string tagWizSuffix = std::string(data.tag.tag) + "." + std::to_string(data.deviceId);
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

/**
* @brief RunProfileData: read datas from new struct data ring buffer to build FileChunkReq
* @param [out] fileChunks: data from user to write to local file or send to remote host
*/
void ReceiveData::RunProfileData(std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks)
{
    switch (profileDataType_) {
        case MSPROF_API_PROFILE_DATA_TYPE:
            RunNoTagData<MsprofApi>(dataBufApi_, fileChunks);
            break;
        case MSPROF_EVENT_PROFILE_DATA_TYPE:
            RunNoTagData<MsprofEvent>(dataBufEvent_, fileChunks);
            break;
        case MSPROF_COMPACT_PROFILE_DATA_TYPE:
            RunTagData<MsprofCompactInfo>(dataBufCompactInfo_, fileChunks);
            break;
        case MSPROF_ADDITIONAL_PROFILE_DATA_TYPE:
            RunTagData<MsprofAdditionalInfo>(dataBufAdditionalInfo_, fileChunks);
            break;
        default:
            MSPROF_LOGE("Entry Run[%s] function is invalid.", moduleName_.c_str());
            break;
    }
}
 
/**
* @brief RunNoTagData: read api/event datas from new struct data ring buffer to build FileChunkReq
* @param [out] fileChunks: data from user to write to local file or send to remote host
*/
template<typename T>
void ReceiveData::RunNoTagData(analysis::dvvp::common::queue::RingBuffer<T> &dataBuffer,
    std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks)
{
    std::vector<T> dataVec; // data binding without tag
    uint64_t batchSizeMax = 0;
    for (; batchSizeMax < MSVP_BATCH_MAX_LEN;) {
        T data;
        bool isOk = dataBuffer.TryPop(data);
        if (!isOk) {
            break;
        }
        totalCountFromRingBuff_++;
        totalDataLengthFromRingBuff_ += sizeof(T);
        batchSizeMax += sizeof(T);
        dataVec.push_back(data);
    }
    if (!dataVec.empty()) {
        SHARED_PTR_ALIA<FileChunkReq> fileChunk = nullptr;
        MSVP_MAKE_SHARED0_BREAK(fileChunk, FileChunkReq);
        MSPROF_LOGD("Dump data, module:%s, data.size:%llu, data[0].len:%llu",
            moduleName_.c_str(), dataVec.size(), sizeof(T));
        if (DumpData(dataVec, fileChunk) == PROFILING_SUCCESS) {
            fileChunks.push_back(fileChunk); // insert the data into the new vector
        } else {
            MSPROF_LOGE("Dump Received Data failed");
        }
    }
}
 
/**
* @brief RunNoTagData: read compact/additional datas from new struct data ring buffer to build FileChunkReq
* @param [out] fileChunks: data from user to write to local file or send to remote host
*/
template<typename T>
void ReceiveData::RunTagData(analysis::dvvp::common::queue::RingBuffer<T> &dataBuffer,
    std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks)
{
    std::map<std::string, std::vector<T>> dataMap; // data binding with tag
    uint64_t batchSizeMax = 0;
    for (; batchSizeMax < MSVP_BATCH_MAX_LEN;) {
        T data;
        bool isOK = dataBuffer.TryPop(data);
        if (!isOK) {
            break;
        }
        totalCountFromRingBuff_++;
        totalDataLengthFromRingBuff_ += sizeof(T);
        batchSizeMax += sizeof(T);
 
        std::string tagWizSuffix = moduleName_ + "." + std::to_string(data.type);
        // classify data by tag.type
        auto iter = dataMap.find(tagWizSuffix);
        if (iter == dataMap.end()) {
            std::vector<T> dataChunkForTag;
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
                moduleName_.c_str(), iter->first.c_str(), iter->second.size(), sizeof(T));
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
    if (moduleName_.find("api") != std::string::npos) {
        dataBufApi_.Init(capacity);
        dataBufApi_.SetName("ReceiveDataApiRing");
        profileDataType_ = MSPROF_API_PROFILE_DATA_TYPE;
    } else if (moduleName_.find("event") != std::string::npos) {
        dataBufEvent_.Init(capacity);
        dataBufEvent_.SetName("ReceiveDataEventRing");
        profileDataType_ = MSPROF_EVENT_PROFILE_DATA_TYPE;
    } else if (moduleName_.find("compact") != std::string::npos) {
        dataBufCompactInfo_.Init(capacity);
        dataBufCompactInfo_.SetName("ReceiveDataCompactInfoRing");
        profileDataType_ = MSPROF_COMPACT_PROFILE_DATA_TYPE;
    } else if (moduleName_.find("additional") != std::string::npos) {
        dataBufAdditionalInfo_.Init(capacity);
        dataBufAdditionalInfo_.SetName("ReceiveDataAdditionalInfoRing");
        profileDataType_ = MSPROF_ADDITIONAL_PROFILE_DATA_TYPE;
    } else {
        dataChunkBuf_.Init(capacity);
        dataChunkBuf_.SetName("ReceiveDataRingBuffer");
    }
    return PROFILING_SUCCESS;
}

int ReceiveData::DoReport(CONST_REPORT_DATA_PTR rData)
{
    ReporterDataChunk dataChunk;
    auto ret = memset_s(&dataChunk, sizeof(dataChunk), 0, sizeof(dataChunk));
    if (ret != EOK) {
        MSPROF_LOGE("memset data chunk ret:%d", ret);
        return PROFILING_FAILED;
    }

    if (!started_ || rData == nullptr) {
        MSPROF_LOGE("module:%s, report failed! started %llu", moduleName_.c_str(), started_);
        return PROFILING_FAILED;
    }
    if (rData->data == nullptr || rData->dataLen <= 0 || rData->dataLen > RECEIVE_CHUNK_SIZE) {
        MSPROF_LOGE("module:%s, report failed! dataLen %llu", moduleName_.c_str(), rData->dataLen);
        return PROFILING_FAILED;
    }
    if (rData->deviceId >= 64 && moduleName_ == "runtime") {  // module runtime, deviceid>=64, return
        MSPROF_LOGW("module:%s, invalid device id:%d", moduleName_.c_str(), rData->deviceId);
        return PROFILING_SUCCESS;
    }
    dataChunk.deviceId = rData->deviceId;
    dataChunk.dataLen = rData->dataLen;
    errno_t err = memcpy_s(dataChunk.tag.tag, MSPROF_ENGINE_MAX_TAG_LEN + 1, rData->tag, MSPROF_ENGINE_MAX_TAG_LEN);
    if (err != EOK) {
        MSPROF_LOGE("memcpy tag failed, err:%d, deviceID:%d, tag:%s, dataLen:%llu",
                    static_cast<int>(err), dataChunk.deviceId, rData->tag, dataChunk.dataLen);
        return PROFILING_FAILED;
    }
    err = memcpy_s(dataChunk.data.data, RECEIVE_CHUNK_SIZE, rData->data, rData->dataLen);
    if (err != EOK) {
        MSPROF_LOGE("memcpy data failed, err:%d, deviceID:%d, tag:%s, dataLen:%llu",
                    static_cast<int>(err), dataChunk.deviceId, rData->tag, dataChunk.dataLen);
        return PROFILING_FAILED;
    }
    return DoReportData(dataChunk);
}

void ReceiveData::DoReportRun()
{
    std::vector<SHARED_PTR_ALIA<FileChunkReq> > fileChunks;
    unsigned long sleepIntevalMs = 500; // 500 : 500ms
    timeStamp_ = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();

    for (;;) {
        fileChunks.clear();
        if (profileDataType_ != MSPROF_DEFAULT_PROFILE_DATA_TYPE) {
            RunProfileData(fileChunks);
        } else {
            Run(fileChunks);
        }
        size_t size = fileChunks.size();
        if (size == 0) {
            SetBufferEmptyEvent();
            if (stopped_) {
                WriteDone();
                MSPROF_LOGI("Exit the Run thread");
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepIntevalMs));
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
        return PROFILING_FAILED;
    } else {
        totalPushCounterSuccess_++;
        totalDataLengthSuccess_ += dataChunk.dataLen;
        return PROFILING_SUCCESS;
    }
}

/**
* @brief DoReportData: get the api data from user and save the data to ring buffer
* @param [in] dataChunk: the data from user
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int ReceiveData::DoReportData(const MsprofApi& dataChunk)
{
    totalPushCounter_.fetch_add(1, std::memory_order_relaxed);
    bool ret = dataBufApi_.TryPush(dataChunk);
    if (!ret) {
        totalPushCounterFailed_++;
        totalDataLengthFailed_ += sizeof(MsprofApi);
        uint64_t totalLengthFailed = totalDataLengthFailed_.load(std::memory_order_relaxed);
        uint64_t totalPushFailed = totalPushCounterFailed_.load(std::memory_order_relaxed);
        size_t buffUsedSize = dataBufApi_.GetUsedSize();
        MSPROF_LOGE("try push ring buff failed, deviceID:%d, module:%s, dataLen:%llu,"
            " totalPushCounterFailed_:%llu, totalDataLengthFailed_:%llu, buffUsedSize:%llu",
            DEFAULT_HOST_ID, moduleName_.c_str(), sizeof(MsprofApi), totalPushFailed, totalLengthFailed, buffUsedSize);
        return PROFILING_FAILED;
    }
    totalPushCounterSuccess_++;
    totalDataLengthSuccess_ += sizeof(MsprofApi);
    return PROFILING_SUCCESS;
}
 
/**
* @brief DoReportData: get the event data from user and save the data to ring buffer
* @param [in] dataChunk: the data from user
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int ReceiveData::DoReportData(const MsprofEvent& dataChunk)
{
    totalPushCounter_.fetch_add(1, std::memory_order_relaxed);
    bool ret = dataBufEvent_.TryPush(dataChunk);
    if (!ret) {
        totalPushCounterFailed_++;
        totalDataLengthFailed_ += sizeof(MsprofEvent);
        uint64_t totalLengthFailed = totalDataLengthFailed_.load(std::memory_order_relaxed);
        uint64_t totalPushFailed = totalPushCounterFailed_.load(std::memory_order_relaxed);
        size_t buffUsedSize = dataBufEvent_.GetUsedSize();
        MSPROF_LOGE("try push ring buff failed, deviceID:%d, module:%s, dataLen:%llu,"
            " totalPushCounterFailed_:%llu, totalDataLengthFailed_:%llu, buffUsedSize:%llu",
            DEFAULT_HOST_ID, moduleName_.c_str(), sizeof(MsprofEvent), totalPushFailed, totalLengthFailed,
            buffUsedSize);
        return PROFILING_FAILED;
    }
    totalPushCounterSuccess_++;
    totalDataLengthSuccess_ += sizeof(MsprofEvent);
    return PROFILING_SUCCESS;
}
 
/**
* @brief DoReportData: get the compact info data from user and save the data to ring buffer
* @param [in] dataChunk: the data from user
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int ReceiveData::DoReportData(const MsprofCompactInfo& dataChunk)
{
    totalPushCounter_.fetch_add(1, std::memory_order_relaxed);
    bool ret = dataBufCompactInfo_.TryPush(dataChunk);
    if (!ret) {
        totalPushCounterFailed_++;
        totalDataLengthFailed_ += sizeof(MsprofCompactInfo);
        uint64_t totalLengthFailed = totalDataLengthFailed_.load(std::memory_order_relaxed);
        uint64_t totalPushFailed = totalPushCounterFailed_.load(std::memory_order_relaxed);
        size_t buffUsedSize = dataBufCompactInfo_.GetUsedSize();
        MSPROF_LOGE("try push ring buff failed, deviceID:%d, module:%s, dataLen:%llu,"
            " totalPushCounterFailed_:%llu, totalDataLengthFailed_:%llu, buffUsedSize:%llu",
            DEFAULT_HOST_ID, moduleName_.c_str(), sizeof(MsprofCompactInfo), totalPushFailed, totalLengthFailed,
            buffUsedSize);
        return PROFILING_FAILED;
    }
    totalPushCounterSuccess_++;
    totalDataLengthSuccess_ += sizeof(MsprofCompactInfo);
    return PROFILING_SUCCESS;
}
 
/**
* @brief DoReportData: get the additional info data from user and save the data to ring buffer
* @param [in] dataChunk: the data from user
* @return : success return PROFILING_SUCCESS, failed return PROFIING_FAILED
*/
int ReceiveData::DoReportData(const MsprofAdditionalInfo& dataChunk)
{
    totalPushCounter_.fetch_add(1, std::memory_order_relaxed);
    bool ret = dataBufAdditionalInfo_.TryPush(dataChunk);
    if (!ret) {
        totalPushCounterFailed_++;
        totalDataLengthFailed_ += sizeof(MsprofAdditionalInfo);
        uint64_t totalLengthFailed = totalDataLengthFailed_.load(std::memory_order_relaxed);
        uint64_t totalPushFailed = totalPushCounterFailed_.load(std::memory_order_relaxed);
        size_t buffUsedSize = dataBufAdditionalInfo_.GetUsedSize();
        MSPROF_LOGE("try push ring buff failed, deviceID:%d, module:%s, dataLen:%llu,"
            " totalPushCounterFailed_:%llu, totalDataLengthFailed_:%llu, buffUsedSize:%llu",
            DEFAULT_HOST_ID, moduleName_.c_str(), sizeof(MsprofAdditionalInfo), totalPushFailed, totalLengthFailed,
            buffUsedSize);
        return PROFILING_FAILED;
    }
    totalPushCounterSuccess_++;
    totalDataLengthSuccess_ += sizeof(MsprofAdditionalInfo);
    return PROFILING_SUCCESS;
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
        CHAR_PTR dataPtr = reinterpret_cast<CHAR_PTR>(&message[i].data.data[0]);
        if (dataPtr == nullptr) {
            return PROFILING_FAILED;
        }
        if (isFirstMessage) { // deal with the data only need to init once
            jobCtx->dev_id = std::to_string(message[i].deviceId);
            jobCtx->tag = std::string(message[i].tag.tag, strlen(message[i].tag.tag));
            fileChunk->set_filename(moduleName_);
            fileChunk->set_offset(-1);
            chunkLen = messageLen;
            chunk = std::string(dataPtr, chunkLen);
            fileChunk->set_islastchunk(false);
            fileChunk->set_needack(false);
            fileChunk->set_tag(std::string(message[i].tag.tag, strlen(message[i].tag.tag)));
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
    jobCtx->dataModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST;
    fileChunk->mutable_hdr()->set_job_ctx(jobCtx->ToString());
    fileChunk->set_chunk(chunk);
    fileChunk->set_chunksizeinbytes(chunkLen);
    return PROFILING_SUCCESS;
}

void ReceiveData::SetDataChunkTag(uint16_t level, uint32_t typeId, std::string &tag) const
{
    switch (profileDataType_) {
        case MSPROF_API_PROFILE_DATA_TYPE:
        case MSPROF_EVENT_PROFILE_DATA_TYPE:
            tag = "data";
            break;
        case MSPROF_COMPACT_PROFILE_DATA_TYPE:
        case MSPROF_ADDITIONAL_PROFILE_DATA_TYPE:
            tag = MsprofReporterMgr::instance()->GetRegReportTypeInfo(level, typeId);
            tag = tag.empty() ? "invalid" : tag;
            break;
        default:
            tag = "invalid";
            break;
    }
}
 
/**
* @brief DumpData: deal with the data from user to build FileChunkReq struct for store or send
* @param [in] message: data saved in the ring buffer
* @param [out] fileChunk: data build from message for store or send
* @return : success return PROFILING_SUCCESS, failed return PROFILING_FAILED
*/
template<typename T>
int ReceiveData::DumpData(std::vector<T> &message, SHARED_PTR_ALIA<FileChunkReq> fileChunk)
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
        int messageLen = sizeof(T);
        CHAR_PTR dataPtr = reinterpret_cast<CHAR_PTR>(&message[i]);
        if (dataPtr == nullptr) {
            return PROFILING_FAILED;
        }
        if (isFirstMessage) { // deal with the data only need to init once
            jobCtx->dev_id = std::to_string(DEFAULT_HOST_ID);
            SetDataChunkTag(message[i].level, message[i].type, jobCtx->tag);
            fileChunk->set_filename(moduleName_);
            fileChunk->set_offset(-1);
            chunkLen = messageLen;
            chunk = std::string(dataPtr, chunkLen);
            fileChunk->set_islastchunk(false);
            fileChunk->set_needack(false);
            fileChunk->set_tag(jobCtx->tag);
            fileChunk->set_tagsuffix(jobCtx->dev_id);
            fileChunk->set_chunkstarttime(0);
            fileChunk->set_chunkendtime(0);
            isFirstMessage = false;
            devIds_.insert(jobCtx->dev_id);
        } else { // deal with the data need to update according every message
            chunk.insert(chunkLen, std::string(dataPtr, messageLen));
            chunkLen += messageLen;
            fileChunk->set_chunkendtime(0);
        }
    }
    jobCtx->module = moduleName_;
    jobCtx->chunkStartTime = fileChunk->chunkstarttime();
    jobCtx->chunkEndTime = fileChunk->chunkendtime();
    jobCtx->dataModule = analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_MSPROF_HOST;
    fileChunk->mutable_hdr()->set_job_ctx(jobCtx->ToString());
    fileChunk->set_chunk(chunk);
    fileChunk->set_chunksizeinbytes(chunkLen);
    return PROFILING_SUCCESS;
}
 
template int ReceiveData::DumpData(std::vector<MsprofApi> &message, SHARED_PTR_ALIA<FileChunkReq> fileChunk);
template int ReceiveData::DumpData(std::vector<MsprofEvent> &message, SHARED_PTR_ALIA<FileChunkReq> fileChunk);
template int ReceiveData::DumpData(std::vector<MsprofCompactInfo> &message,
                                   SHARED_PTR_ALIA<FileChunkReq> fileChunk);
template int ReceiveData::DumpData(std::vector<MsprofAdditionalInfo> &message,
                                   SHARED_PTR_ALIA<FileChunkReq> fileChunk);
 
template void ReceiveData::RunNoTagData(analysis::dvvp::common::queue::RingBuffer<MsprofApi> &dataBuffer,
    std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks);
template void ReceiveData::RunNoTagData(analysis::dvvp::common::queue::RingBuffer<MsprofEvent> &dataBuffer,
    std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks);
template void ReceiveData::RunTagData(analysis::dvvp::common::queue::RingBuffer<MsprofCompactInfo> &dataBuffer,
    std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks);
template void ReceiveData::RunTagData(analysis::dvvp::common::queue::RingBuffer<MsprofAdditionalInfo> &dataBuffer,
    std::vector<SHARED_PTR_ALIA<FileChunkReq>>& fileChunks);
}}
