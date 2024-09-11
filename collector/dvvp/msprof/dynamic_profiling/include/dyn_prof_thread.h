/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: dynamic profiling thread
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-11-28
 */
 
#ifndef COLLECTOR_DYNAMIC_PROFILING_THREAD_H
#define COLLECTOR_DYNAMIC_PROFILING_THREAD_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "thread/thread.h"
#include "prof_api.h"
#include "msprof_error_manager.h"

namespace Collector {
namespace Dvvp {
namespace DynProf {

class DynProfThread : public analysis::dvvp::common::thread::Thread {
public:
    DynProfThread();
    ~DynProfThread();

    int Start() override;
    int Stop() override;
    void Run(const struct error_message::Context &errorContext) override;

    void SaveDevicesInfo(ProfSetDevPara data);

private:
    int GetDelayAndDurationTime();
    int StartProfTask();
    int HandleDevProfTask(const ProfSetDevPara &devInfo);
    int StopProfTask();

private:
    bool started_;
    uint32_t delayTime_;
    uint32_t durationTime_;
    bool durationSet_;
    std::condition_variable cvThreadStop_;
    std::mutex threadStopMtx_;
    std::atomic<bool> profHasStarted_;
    std::mutex deviceMtx_;
    std::map<uint32_t, ProfSetDevPara> deviceInfos_;
    std::string msprofEnvCfg_;
};
} // DynProf
} // Dvvp
} // Collect
#endif