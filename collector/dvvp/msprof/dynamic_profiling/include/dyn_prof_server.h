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