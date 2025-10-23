/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-12-10
 */
#ifndef COLLECTOR_DYNAMIC_PROFILING_SERVER_H
#define COLLECTOR_DYNAMIC_PROFILING_SERVER_H

#include <atomic>
#include <mutex>
#include "thread/thread.h"
#include "dyn_prof_common.h"
#include "prof_api.h"
namespace Collector {
namespace Dvvp {
namespace DynProf {

class DynProfServer : public analysis::dvvp::common::thread::Thread {
public:
    DynProfServer();
    ~DynProfServer() override;
    int Start() override;
    int Stop() override;
    void NotifyClientDisconnect(const std::string &detailInfo);
    void SaveDevicesInfo(ProfSetDevPara data);

private:
    int DynProfServerCreateSock();
    bool IdleConnectOverTime(uint32_t &recvIdleTimes) const;
    void DynProfServerProcMsg();
    void DynProfSrvProcStart();
    void DynProfSrvProcStop();
    void DynProfSrvProcQuit();
    int DynProfServerRsqMsg(DynProfMsgType msgType, DynProfMsgProcRes rsqCode, const std::string &msgData);
    void Run(const struct error_message::Context &errorContext) override;
    int DynProfSrvProcStartDeviceTask(std::string &detailInfo);
private:
    int srvSockFd_;
    int cliSockFd_;
    bool srvStarted_;
    std::atomic<bool> profHasStarted_;
    uint32_t startTimes_;
    std::string recvParams_;
    std::string sockPath_;
    std::mutex deviceMtx_;
    std::map<uint32_t, ProfSetDevPara> deviceInfos_;
};

}
}
}
#endif // COLLECTOR_DYNAMIC_PROFILING_SERVER_H