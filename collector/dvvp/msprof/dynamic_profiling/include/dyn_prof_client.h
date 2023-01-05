/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-12-10
 */
#ifndef COLLECTOR_DYNAMIC_PROFILING_CLIENT_H
#define COLLECTOR_DYNAMIC_PROFILING_CLIENT_H
#include <functional>
#include "dyn_prof_common.h"
#include "thread/thread.h"

namespace Collector {
namespace Dvvp {
namespace DynProf {

using ProcFunc = std::function<std::string()>;

class DyncProfMsgProcCli : public analysis::dvvp::common::thread::Thread {
public:
    DyncProfMsgProcCli();
    ~DyncProfMsgProcCli() override;
    int Start() override;
    int Stop() override;
    int SetParams(const std::string &params);

private:
    std::string DynProfCliProcHelp();
    int CreateDynProfClientSock();
    int SendMsgToServer(DynProfMsgType reqMsgtype, DynProfMsgType rsqMsgtype,
        const std::string &reqMsgParams, std::string &echoTips);
    int TryReadInputCmd(std::string &inputCmd);
    bool IsServerDisconnect(std::string &echoTips);
    void Run(const struct error_message::Context &errorContext) override;

private:
    bool cliStarted_;
    int cliSockFd_;
    std::string sendParams_;
};

class DynProfMngCli : public analysis::dvvp::common::singleton::Singleton<DynProfMngCli> {
public:
    DynProfMngCli();
    ~DynProfMngCli();
    int StartDynProfCli(const std::string &params);
    void StopDynProfCli();
    int GetRealAppPid(int pid);
    void SetAppPid(int pid);
    int GetAppPid();
    void EnableMode();
    bool IsEnableMode();
    std::string ConstructEnv();
    void WaitQuit();

private:
    bool enabled_;
    int appPid_;
    SHARED_PTR_ALIA<Collector::Dvvp::DynProf::DyncProfMsgProcCli> dynProfCli_;
};
}
}
}
#endif // COLLECTOR_DYNAMIC_PROFILING_CLIENT_H