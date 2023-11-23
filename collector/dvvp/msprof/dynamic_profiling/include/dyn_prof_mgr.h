/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-11-10
 */
 
#ifndef COLLECTOR_DYNAMIC_PROFILING_H
#define COLLECTOR_DYNAMIC_PROFILING_H
 
#include <string>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "singleton/singleton.h"
#include "thread/thread.h"
#include "msprof_error_manager.h"
#include "message/prof_params.h"
#include "prof_callback.h"
#include "prof_api.h"
#include "dyn_prof_server.h"
 
namespace Collector {
namespace Dvvp {
namespace DynProf {
 
using namespace analysis::dvvp::common::thread;
 
class DynProfMgr : public analysis::dvvp::common::singleton::Singleton<DynProfMgr>,
                   public analysis::dvvp::common::thread::Thread {
public:
    DynProfMgr();
    ~DynProfMgr();
public:
    int StartDynProf();
    void StopDynProf();
    bool IsDynProfStarted();
    void SetDeviceInfo(ProfSetDevPara *data);
    int StartDynProfSrv();
 
    int Start() override;
    int Stop() override;
    void Run(const struct error_message::Context &errorContext) override;
 
private:
    int StartProfTask();
    int StartDevProfTask(const ProfSetDevPara &devInfo);
    int StopProfTask();
 
private:
    uint32_t delayTime_;
    uint32_t durationTime_;
    bool durationSet_;
    bool isStarted_;
    std::mutex startMtx_;
    std::condition_variable threadStop_;
    std::mutex threadStopMtx_;
    std::vector<ProfSetDevPara> devicesInfo_;
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv_;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
    std::string msprofEnvCfg_;
};
} // DynProf
} // Dvvp
} // Collect
#endif
