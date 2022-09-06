/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_TRANSPORT_PROF_CHANNEL_H
#define ANALYSIS_DVVP_TRANSPORT_PROF_CHANNEL_H

#include <map>
#include <memory>
#include <mutex>
#include "ai_drv_dev_api.h"
#include "ai_drv_prof_api.h"
#include "message/prof_params.h"
#include "statistics/perf_count.h"
#include "thread/thread.h"
#include "thread/thread_pool.h"

namespace analysis {
namespace dvvp {
namespace transport {
using namespace Analysis::Dvvp::Common::Statistics;
class ChannelReader : public analysis::dvvp::common::thread::Task {
public:
    ChannelReader(int deviceId,
                  analysis::dvvp::driver::AI_DRV_CHANNEL channelId,
                  const std::string &relativeFileName,
                  SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx);
    virtual ~ChannelReader();

public:
    virtual int Execute();
    virtual size_t HashId();

public:
    int Init();
    int Uinit();
    void UploadData();
    void FlushBuffToUpload();
    void SetChannelStopped();
    bool GetSchedulingStatus() const;
    void SetSchedulingStatus(bool isScheduling);
    void FlushDrvBuff();
    void CheckIfSendFlush(const size_t curLen);
    void SendFlushFinished();

private:
    int deviceId_;
    analysis::dvvp::driver::AI_DRV_CHANNEL channelId_;
    std::string relativeFileName_;
    unsigned int bufSize_;
    unsigned int dataSize_;
    unsigned int spaceSize_;
    SHARED_PTR_ALIA<unsigned char> buffer_;
    size_t hashId_;
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx_;
    long long totalSize_;
    volatile bool isChannelStopped_;
    volatile bool isInited_;
    SHARED_PTR_ALIA<PerfCount> readSpeedPerfCount_;
    SHARED_PTR_ALIA<PerfCount> overallReadSpeedPerfCount_;
    uint64_t lastEndRawTime_;
    uint64_t drvChannelReadCont_;
    std::mutex mtx_;
    volatile bool isScheduling_;
    // flush
    bool needWait_;
    unsigned int flushBufSize_;
    unsigned int flushCurSize_;
    std::mutex flushMutex_;
    std::condition_variable flushFlag_;
};

class ChannelPoll : public analysis::dvvp::common::thread::Thread {
public:
    ChannelPoll();
    virtual ~ChannelPoll();

public:
    int AddReader(unsigned int devId, unsigned int channelId, SHARED_PTR_ALIA<ChannelReader> reader);
    int RemoveReader(unsigned int devId, unsigned int channelId);
    SHARED_PTR_ALIA<ChannelReader> GetReader(unsigned int devId, unsigned int channelId);
    int DispatchChannel(unsigned int devId, unsigned int channelId);
    void FlushDrvBuff();

public:
    int Start() override;
    int Stop() override;

protected:
    void Run(const struct error_message::Context &errorContext) override;

private:
    std::vector<SHARED_PTR_ALIA<ChannelReader>> GetAllReaders();
    void DispatchReader(SHARED_PTR_ALIA<ChannelReader> reader);
    void FlushAllChannels();

private:
    SHARED_PTR_ALIA<analysis::dvvp::common::thread::ThreadPool> threadPool_;
    std::map<unsigned int, std::map<unsigned int, SHARED_PTR_ALIA<ChannelReader>>> readers_;
    std::mutex mtx_;
    volatile bool isStarted_;
};
}  // namespace device
}  // namespace dvvp
}  // namespace analysis

#endif
