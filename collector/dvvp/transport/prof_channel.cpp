/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: lixubo
 * Create: 2018-06-13
 */
#include "prof_channel.h"
#include <functional>
#include <new>
#include <string>
#include "config/config.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "msprof_dlog.h"
#include "ascend_hal.h"
#include "proto/profiler.pb.h"
#include "securec.h"
#include "uploader_mgr.h"

namespace analysis {
namespace dvvp {
namespace transport {
constexpr unsigned int MAX_BUFFER_SIZE = 1024 * 1024 * 2;
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::MsprofErrMgr;

ChannelReader::ChannelReader(int deviceId,
                             analysis::dvvp::driver::AI_DRV_CHANNEL channelId,
                             const std::string &relativeFileName,
                             SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx)
    : deviceId_(deviceId),
      channelId_(channelId),
      relativeFileName_(relativeFileName),
      bufSize_(MAX_BUFFER_SIZE),
      dataSize_(0),
      spaceSize_(MAX_BUFFER_SIZE),
      buffer_(nullptr),
      hashId_(0),
      jobCtx_(jobCtx),
      totalSize_(0),
      isChannelStopped_(false),
      isInited_(false),
      readSpeedPerfCount_(nullptr),
      overallReadSpeedPerfCount_(nullptr),
      lastEndRawTime_(0),
      drvChannelReadCont_(0),
      isScheduling_(false),
      needWait_(false),
      flushBufSize_(0),
      flushCurSize_(0)
{
}

ChannelReader::~ChannelReader()
{
}

int ChannelReader::Init()
{
    hashId_ = std::hash<std::string>()(relativeFileName_ +
        std::to_string(static_cast<int>(channelId_)) +
        std::to_string(deviceId_) +
        std::to_string(analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw()));
    MSVP_MAKE_SHARED_ARRAY_RET(buffer_, unsigned char, bufSize_, PROFILING_FAILED);
    MSVP_MAKE_SHARED1_RET(readSpeedPerfCount_, PerfCount, SPEED_PERFCOUNT_MODULE_NAME, PROFILING_FAILED);
    MSVP_MAKE_SHARED1_RET(overallReadSpeedPerfCount_, PerfCount, SPEEDALL_PERFCOUNT_MODULE_NAME, PROFILING_FAILED);
    lastEndRawTime_ = 0;
    isInited_ = true;
    std::lock_guard<std::mutex> lk(mtx_);
    drvChannelReadCont_ = 0;
    return PROFILING_SUCCESS;
}

int ChannelReader::Uinit()
{
    MSPROF_EVENT("device id %d, channel: %d, total_size_channel: %lld, file:%s, job_id:%s, drvChannelReadCont:%lld",
        deviceId_, static_cast<int>(channelId_), totalSize_,
        relativeFileName_.c_str(), jobCtx_->job_id.c_str(), drvChannelReadCont_);
    std::string tag = "[" +  jobCtx_->job_id + " : " +
        std::to_string(deviceId_) + " : " + std::to_string(channelId_) + "]";
    readSpeedPerfCount_->OutPerfInfo("ChannelReaderSpeed" + tag);
    overallReadSpeedPerfCount_->OutPerfInfo("ChannelReaderSpeedAll" + tag);
    isInited_ = false;
    do {
        MSVP_TRY_BLOCK_BREAK({
            FlushBuffToUpload();
        });
    } while (0);

    return PROFILING_SUCCESS;
}

void ChannelReader::SetChannelStopped()
{
    isChannelStopped_ = true;
}

bool ChannelReader::GetSchedulingStatus() const
{
    return isScheduling_;
}

void ChannelReader::SetSchedulingStatus(bool isScheduling)
{
    isScheduling_ = isScheduling;
}

int ChannelReader::Execute()
{
    int totalLen = 0;
    int currLen = 0;
    static const int maxReadLen = 1024 * 1024 * 32;
    static const int maxThresholdSize = static_cast<int>(MAX_BUFFER_SIZE * 0.8);
    SetSchedulingStatus(false);
    std::lock_guard<std::mutex> lk(mtx_);
    std::unique_lock<std::mutex> guard(flushMutex_, std::defer_lock);
    do {
        drvChannelReadCont_++;
        if (!isInited_ || isChannelStopped_) {
            break;
        }
        if ((dataSize_ > maxThresholdSize)) {
            UploadData();
        }

        spaceSize_ = bufSize_ - dataSize_;
        uint64_t startRawTime = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
        guard.lock();
        currLen = DrvChannelRead(deviceId_, channelId_, buffer_.get() + dataSize_, spaceSize_);
        CheckIfSendFlush(currLen);
        guard.unlock();
        MSPROF_LOGD("deviceId:%d, channelId:%d fileName:%s, bufSize:%u, currLen:%d, spaceSize:%u",
            deviceId_, channelId_, relativeFileName_.c_str(), bufSize_, currLen, spaceSize_);

        if (currLen <= 0) {
            if (currLen < 0) {
                MSPROF_LOGE("read device %d, channel:%d, ret=%d",
                    deviceId_, static_cast<int>(channelId_), currLen);
                MSPROF_INNER_ERROR("EK9999", "read device %d, channel:%d, ret=%d",
                    deviceId_, static_cast<int>(channelId_), currLen);
            }
            break;
        }

        uint64_t endRawTime = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
        readSpeedPerfCount_->UpdatePerfInfo(startRawTime, endRawTime, currLen); // update the PerfCount info
        if (lastEndRawTime_ != 0) {
            overallReadSpeedPerfCount_->UpdatePerfInfo(lastEndRawTime_, endRawTime, currLen);
        }
        lastEndRawTime_ = endRawTime;

        totalLen += currLen;
        totalSize_ += static_cast<long long>(currLen);

        dataSize_ += static_cast<uint32_t>(currLen);
    } while (static_cast<unsigned int>(currLen) == (spaceSize_) && (totalLen < maxReadLen));

    return PROFILING_SUCCESS;
}

size_t ChannelReader::HashId()
{
    return hashId_;
}

void ChannelReader::UploadData()
{
    if (dataSize_ == 0) {
        return;
    }
    // file chunk
    SHARED_PTR_ALIA<analysis::dvvp::proto::FileChunkReq> fileChunkReq;
    MSVP_MAKE_SHARED0_VOID(fileChunkReq, analysis::dvvp::proto::FileChunkReq);

    fileChunkReq->set_filename(relativeFileName_);
    fileChunkReq->set_offset(-1);
    fileChunkReq->set_chunk(buffer_.get(), dataSize_);
    fileChunkReq->set_chunksizeinbytes(dataSize_);
    fileChunkReq->set_islastchunk(false);
    fileChunkReq->set_needack(false);
    fileChunkReq->mutable_hdr()->set_job_ctx(jobCtx_->ToString());
    fileChunkReq->set_datamodule(analysis::dvvp::common::config::FileChunkDataModule::PROFILING_IS_FROM_DEVICE);

    std::string encoded = analysis::dvvp::message::EncodeMessage(fileChunkReq);
    int ret = UploaderMgr::instance()->UploadData(jobCtx_->job_id,
        reinterpret_cast<const void *>(encoded.c_str()), (uint32_t)encoded.size());
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Upload data failed, jobId: %s", jobCtx_->job_id.c_str());
        MSPROF_INNER_ERROR("EK9999", "Upload data failed, jobId: %s", jobCtx_->job_id.c_str());
    }
    dataSize_ = 0;
}

void ChannelReader::FlushBuffToUpload()
{
    std::lock_guard<std::mutex> lk(mtx_);
    UploadData();
}

void ChannelReader::FlushDrvBuff()
{
    if ((channelId_ != PROF_CHANNEL_HWTS_LOG) && (channelId_ != PROF_CHANNEL_TS_FW)) {
        return;
    }
    // 1. query flush size
    std::unique_lock<std::mutex> guard(flushMutex_);
    unsigned int flushSize = 0;
    int ret = DrvProfFlush(deviceId_, channelId_, flushSize);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("DrvProfFlush failed, deviceId:%d, channelId:%d, ret:%d", deviceId_, channelId_, ret);
        MSPROF_INNER_ERROR("EK9999", "DrvProfFlush failed, deviceId:%d, channelId:%d,ret:%d",
            deviceId_, channelId_, ret);
        guard.unlock();
        return;
    }
    MSPROF_LOGI("Flush deviceId:%d, channelId:%d, flushSize:%d", deviceId_, channelId_, flushSize);
    // 2. wait flush finished
    if (flushSize == 0) {
        MSPROF_LOGI("No drv data need flush.");
        guard.unlock();
        return;
    }
    needWait_ = true;
    flushBufSize_ = flushSize;
    flushFlag_.wait(guard, [=] { return !needWait_; });
    // 3. upload flush data
    FlushBuffToUpload();
}

void ChannelReader::CheckIfSendFlush(const size_t curLen)
{
    if ((channelId_ != PROF_CHANNEL_HWTS_LOG) && (channelId_ != PROF_CHANNEL_TS_FW)) {
        return;
    }
    if (needWait_) {
        if (flushCurSize_ > UINT_MAX - curLen) { // Check for overflow, if curLen is very large,Sure to send finish
            SendFlushFinished();
        } else {
            flushCurSize_ += curLen;
            if (flushBufSize_ <= flushCurSize_ || curLen == 0) {
                SendFlushFinished();
            }
        }
    }
}

void ChannelReader::SendFlushFinished()
{
    needWait_ = false;
    flushCurSize_ = 0;
    flushBufSize_ = 0;
    flushFlag_.notify_all();
}

ChannelPoll::ChannelPoll(): threadPool_(nullptr), isStarted_(false)
{
}

ChannelPoll::~ChannelPoll()
{
    Stop();
}

int ChannelPoll::AddReader(unsigned int devId, unsigned int channelId, SHARED_PTR_ALIA<ChannelReader> reader)
{
    if (reader == nullptr) {
        return PROFILING_FAILED;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    MSPROF_LOGI("AddReader, devId:%u, channel:%u", devId, channelId);
    readers_[devId][channelId] = reader;

    return PROFILING_SUCCESS;
}

int ChannelPoll::RemoveReader(unsigned int devId, unsigned int channelId)
{
    std::lock_guard<std::mutex> lk(mtx_);
    MSPROF_LOGI("RemoveReader, devId:%u, channel:%u", devId, channelId);
    auto iter = readers_.find(devId);
    if (iter != readers_.end()) {
        MSPROF_LOGI("RemoveReader, fid dev, devId:%u", devId);
        auto channelIter = iter->second.find(channelId);
        if (channelIter != iter->second.end()) {
            MSPROF_LOGI("RemoveReader, devId:%u, channel:%u", devId, channelId);
            channelIter->second->SetChannelStopped();
            channelIter->second->Uinit();
            iter->second.erase(channelIter);
        }
        if (iter->second.size() == 0) {
            readers_.erase(iter);
        }
    }

    return PROFILING_SUCCESS;
}

SHARED_PTR_ALIA<ChannelReader> ChannelPoll::GetReader(unsigned int devId, unsigned int channelId)
{
    SHARED_PTR_ALIA<ChannelReader> reader = nullptr;

    std::lock_guard<std::mutex> lk(mtx_);
    auto iter = readers_.find(devId);
    if (iter != readers_.end()) {
        auto channelIter = iter->second.find(channelId);
        if (channelIter != iter->second.end()) {
            reader = channelIter->second;
        }
    }

    return reader;
}

std::vector<SHARED_PTR_ALIA<ChannelReader>> ChannelPoll::GetAllReaders()
{
    std::vector<SHARED_PTR_ALIA<ChannelReader> > res;

    std::lock_guard<std::mutex> lk(mtx_);
    for (auto iter = readers_.begin(); iter != readers_.end(); ++iter) {
        for (auto channelIter = iter->second.begin(); channelIter != iter->second.end(); ++channelIter) {
            res.push_back(channelIter->second);
        }
    }

    return res;
}

void ChannelPoll::DispatchReader(SHARED_PTR_ALIA<ChannelReader> reader)
{
    bool isScheduling = reader->GetSchedulingStatus();
    if (!isScheduling) {
        reader->SetSchedulingStatus(true);
        (void)threadPool_->Dispatch(reader);
    }
}

int ChannelPoll::DispatchChannel(unsigned int devId, unsigned int channelId)
{
    auto reader = GetReader(devId, channelId);
    if (!reader) {
        MSPROF_LOGW("Failed to find devId:%u, channel:%u", devId, channelId);
        return PROFILING_FAILED;
    }
    DispatchReader(reader);

    return PROFILING_SUCCESS;
}

void ChannelPoll::FlushAllChannels()
{
    auto res = GetAllReaders();
    for (auto iter = res.begin(); iter != res.end(); ++iter) {
        (*iter)->FlushBuffToUpload();
    }
}

void ChannelPoll::FlushDrvBuff()
{
    auto res = GetAllReaders();
    for (auto iter = res.begin(); iter != res.end(); ++iter) {
        (*iter)->FlushDrvBuff();
    }
}

int ChannelPoll::Start()
{
    int devNum = DrvGetDevNum();
    MSPROF_LOGI("Get device num %d", devNum);
    int threadPoolNum = devNum; // at least one thread
    if (devNum <= 0) {
        threadPoolNum = 1;
    } else if (devNum > DEV_NUM) {
        threadPoolNum = DEV_NUM;
    }

    MSVP_MAKE_SHARED2_RET(threadPool_, analysis::dvvp::common::thread::ThreadPool,
                          analysis::dvvp::common::thread::LOAD_BALANCE_METHOD::ID_MOD, threadPoolNum, PROFILING_FAILED);
    threadPool_->SetThreadPoolNamePrefix(MSVP_CHANNEL_POOL_NAME_PREFIX);
    threadPool_->SetThreadPoolQueueSize(CHANNELPOLL_THREAD_QUEUE_SIZE);
    isStarted_ = true;
    (void) threadPool_->Start();
    analysis::dvvp::common::thread::Thread::SetThreadName(msvpChannelThreadName);
    isStarted_ = true;
    (void)analysis::dvvp::common::thread::Thread::Start();
    return PROFILING_SUCCESS;
}

int ChannelPoll::Stop()
{
    if (isStarted_) {
        isStarted_ = false;
        (void)analysis::dvvp::common::thread::Thread::Stop();
        (void)threadPool_->Stop();
    }

    return PROFILING_SUCCESS;
}

void ChannelPoll::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    static const unsigned int channelNum = 6; // at most get 6 channels to read once loop
    static const int defaultTimeoutSec = 1; // at most wait for 1 seconds
    static const uint64_t defaultFlushTimeoutNsec = 10000000000; // flush data every 10 seconds

    struct prof_poll_info channels[channelNum];
    (void)memset_s(channels, channelNum * sizeof(struct prof_poll_info), 0,
                   channelNum * sizeof(struct prof_poll_info));

    uint64_t start = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
    while (isStarted_) {
        int ret = DrvChannelPoll(channels,
                                 channelNum,
                                 defaultTimeoutSec);
        if (ret == PROF_ERROR) {
            MSPROF_LOGE("Failed to poll channel");
            MSPROF_INNER_ERROR("EK9999", "Failed to poll channel");
            break;
        }
        if (ret == PROF_STOPPED_ALREADY) {
            MSPROF_LOGD("drvChannelPoll return PROF_STOPPED_ALREADY");
            if (IsQuit()) {
                MSPROF_LOGI("Exit poll channel thread.");
                break;
            } else {
                const unsigned long sleepTimeInUs = 1000; // 1000us
                analysis::dvvp::common::utils::Utils::UsleepInterupt(sleepTimeInUs);
                continue;
            }
        }

        uint64_t end = analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw();
        if ((end - start) >= defaultFlushTimeoutNsec) {
            start = end;
            FlushAllChannels();
        }

        for (int ii = 0; ii < ret; ++ii) {
            MSPROF_LOGD("DispatchChannel devId: %d, channelID: %d",
                channels[ii].device_id, channels[ii].channel_id);
            (void)DispatchChannel(channels[ii].device_id, channels[ii].channel_id);
        }
    }
}
}  // namespace device
}  // namespace dvvp
}  // namespace analysis
