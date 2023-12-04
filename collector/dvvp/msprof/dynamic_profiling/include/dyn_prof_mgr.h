/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2023-11-10
 */
 
#ifndef COLLECTOR_DYNAMIC_PROFILING_MGR_H
#define COLLECTOR_DYNAMIC_PROFILING_MGR_H
 
#include <string>
#include <mutex>
#include "singleton/singleton.h"
#include "dyn_prof_server.h"
#include "dyn_prof_thread.h"
 
namespace Collector {
namespace Dvvp {
namespace DynProf {
 
using namespace analysis::dvvp::common::thread;
 
class DynProfMgr : public analysis::dvvp::common::singleton::Singleton<DynProfMgr> {
public:
    DynProfMgr();
    ~DynProfMgr();
public:
    int StartDynProf();
    void StopDynProf();
    void SaveDevicesInfo(ProfSetDevPara data) const;
    bool IsDynProfStarted();

private:
    bool isStarted_;
    std::mutex startMtx_;
    SHARED_PTR_ALIA<DynProfServer> dynProfSrv_;
    SHARED_PTR_ALIA<DynProfThread> dynProfThread_;
};
} // DynProf
} // Dvvp
} // Collect
#endif
