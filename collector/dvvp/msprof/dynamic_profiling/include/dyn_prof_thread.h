/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
 
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
    void ReleaseProfTask(const std::vector<uint32_t>& devIds);

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