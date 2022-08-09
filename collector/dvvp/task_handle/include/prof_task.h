/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: handle profiling request
 * Author: hufengwei
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_HOST_PROF_TASK_H
#define ANALYSIS_DVVP_HOST_PROF_TASK_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include "device.h"
#include "proto/profiler.pb.h"
#include "queue/bound_queue.h"
#include "thread/thread.h"
#include "uploader.h"
#include "utils/utils.h"
namespace analysis {
namespace dvvp {
namespace host {
using namespace analysis::dvvp::common::utils;
using StreamQueue = analysis::dvvp::common::queue::BoundQueue<SHARED_PTR_ALIA<std::string> >;

class ProfTask : public analysis::dvvp::common::thread::Thread {
public:
    ProfTask(const std::vector<std::string> &devices,
             SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> param);
    virtual ~ProfTask();

public:
    int Init();
    int Uinit();
    bool IsDeviceRunProfiling(const std::string &devStr);

public:
    void Run(const struct error_message::Context &errorContext) override;
    int Stop() override;

public:
    int NotifyFileDoneForDevice(const std::string &fileName, const std::string &devId) const;
    int WriteStreamData(CONST_VOID_PTR data, unsigned int dataLen) const;
    void SetIsFinished(bool finished);
    bool GetIsFinished() const;
    int SendMsgAndHandleResponse(int devId, SHARED_PTR_ALIA<google::protobuf::Message> msg);

private:
    void StoreResultStatus();
    void WriteDone();

private:
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;

    std::vector<std::string> _devices_v;
    std::vector<std::string> currDevicesV_;

    std::mutex taskMtx_;
    std::condition_variable cv_;
    std::mutex devicesMtx_;
    std::map<std::string, SHARED_PTR_ALIA<Device> > devicesMap_;

    SHARED_PTR_ALIA<analysis::dvvp::message::Status> taskStatus_;

    bool isInited_;
    std::string startTime_;

    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> uploader_;

    bool isFinished_;

private:
    int GetHostAndDeviceInfo();
    std::string GetHostTime();
    void GenerateFileName(bool isStartTime, std::string &filename);
    void SaveRankId(SHARED_PTR_ALIA<analysis::dvvp::proto::CollectionStartEndTime> timeInfo);
    int CreateCollectionTimeInfo(std::string collectionTime, bool isStartTime);
    void StartDevices(const std::vector<std::string> &devicesVec);
    void ProcessDefMode();
    void FinishJobRsp(SHARED_PTR_ALIA<analysis::dvvp::message::Status> taskStatus);
    std::string GetDevicesStr(const std::vector<std::string> &events);
    void Process(analysis::dvvp::message::StatusInfo &statusInfo);
};
}  // namespace host
}  // namespace dvvp
}  // namespace analysis
#endif
