/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-12-10
 */
#ifndef COLLECTOR_DYNAMIC_PROFILING_SERVER_H
#define COLLECTOR_DYNAMIC_PROFILING_SERVER_H
#include "thread/thread.h"
#include "dyn_prof_common.h"
#include "prof_api.h"
namespace Collector {
namespace Dvvp {
namespace DynProf {
 
class DyncProfMsgProcSrv : public analysis::dvvp::common::thread::Thread {
public:
    DyncProfMsgProcSrv();
    ~DyncProfMsgProcSrv() override;
    int Start() override;
    int Stop() override;
 
private:
    int DynProfServerCreateSock();
    void DynProfServerProcMsg();
    void DynProfSrvProcStart();
    void DynProfSrvProcStop();
    void DynProfSrvProcQuit();
    void DynProfServerRsqMsg(DynProfMsgType msgType, DynProfMsgRsqCode rsqCode);
    void Run(const struct error_message::Context &errorContext) override;
 
private:
    int srvSockFd_;
    int cliSockFd_;
    bool srvStarted_;
    bool profHasStarted_;
    std::string recvParams_;
};
 
class DynProfMngSrv : public analysis::dvvp::common::singleton::Singleton<DynProfMngSrv> {
public:
    DynProfMngSrv();
    ~DynProfMngSrv();
    int StartDynProfSrv();
    void StopDynProfSrv();
    void SetDeviceInfo(ProfSetDevPara *data);
    ProfSetDevPara *GetDeviceInfo();
    bool IsStarted();
 
public:
    int startTimes_;
 
private:
    bool started_;
    ProfSetDevPara deviceInfo_;
    SHARED_PTR_ALIA<Collector::Dvvp::DynProf::DyncProfMsgProcSrv> dynProfSrv_;
};
 
}
}
}
#endif // COLLECTOR_DYNAMIC_PROFILING_SERVER_H