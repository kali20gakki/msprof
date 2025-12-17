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
    void SaveDevicesInfo(ProfSetDevPara data);
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
