/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-12-10
 */
#include "dyn_prof_server.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "errno/error_code.h"
#include "dyn_prof_common.h"
#include "prof_acl_mgr.h"
#include "msprof_callback_impl.h"
#include "mmpa_api.h"
#include "utils/utils.h"

namespace Collector {
namespace Dvvp {
namespace DynProf {
using namespace Analysis::Dvvp::ProfilerCommon;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;
using namespace analysis::dvvp::common::error;

DyncProfMsgProcSrv::DyncProfMsgProcSrv() : srvStarted_(false), srvSockFd_(-1), cliSockFd_(-1), profHasStarted_(false)
{
}

DyncProfMsgProcSrv::~DyncProfMsgProcSrv()
{
}

int DyncProfMsgProcSrv::Start()
{
    MSPROF_LOGI("Dynamic profiling begin to init server.");
    if (srvStarted_) {
        MSPROF_LOGW("Dynamic profiling server thread has been started.");
        return PROFILING_SUCCESS;
    }
    if (DynProfServerCreateSock() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling create server socket fail.");
        return PROFILING_FAILED;
    }
    srvStarted_ = true;
    Thread::SetThreadName(analysis::dvvp::common::config::MSVP_DYN_PROF_SERVER_THREAD_NAME);
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        Stop();
        MSPROF_LOGE("Dynamic profiling start thread fail.");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Dynamic profiling init server success.");
    return PROFILING_SUCCESS;
}

int DyncProfMsgProcSrv::Stop()
{
    MSPROF_LOGI("Dynamic profiling begin to stop server.");
    if (!srvStarted_) {
        MSPROF_LOGW("Dynamic profiling server not started.");
        return PROFILING_SUCCESS;
    }
    MmUnlink(sockPath_);
    srvStarted_ = false;
    Utils::MsleepInterruptible(DYN_PROF_SERVER_RECV_WAIT_TIME * MMPA_SEC_TO_MSEC);
    if (srvSockFd_ > 0) {
        close(srvSockFd_);
        srvSockFd_ = -1;
    }
    if (cliSockFd_ > 0) {
        close(cliSockFd_);
        cliSockFd_ = -1;
    }
    MSPROF_LOGI("Dynamic profiling stop server success.");
    return PROFILING_SUCCESS;
}

void DyncProfMsgProcSrv::Run(const struct error_message::Context &errorContext)
{
    timeval timeout = {DYN_PROF_SERVER_ACCEPT_WAIT_TIME, 0};
    (void)setsockopt(srvSockFd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<CHAR_PTR>(&timeout), sizeof(struct timeval));

    uint32_t acceptTimes = 0;
    sockaddr_un clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    while (srvStarted_ && (acceptTimes < DYN_PROF_MAX_ACCEPT_TIMES)) {
        cliSockFd_ = accept(srvSockFd_, reinterpret_cast<sockaddr *>(&clt_addr), &clt_addr_len);
        if (cliSockFd_ < 0) {
            if (errno == EAGAIN) {
                continue;   // timeout
            } else {
                MSPROF_LOGE("Dynamic profiling accept failed, cliSockFd_=%d errno=%u.", cliSockFd_, errno);
                break;
            }
        }
        acceptTimes++;
        timeval timeout2 = {DYN_PROF_SERVER_RECV_WAIT_TIME, 0};
        (void)setsockopt(cliSockFd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<CHAR_PTR>(&timeout2),
            sizeof(struct timeval));
        DynProfServerProcMsg();
        close(cliSockFd_);
        cliSockFd_ = -1;
    }
    if (acceptTimes >= DYN_PROF_MAX_ACCEPT_TIMES) {
        MSPROF_LOGW("Dynamic profiling accept times over %u, stop dynamic profiling.", DYN_PROF_MAX_ACCEPT_TIMES);
        Stop();
    }
}

int DyncProfMsgProcSrv::DynProfServerCreateSock()
{
    int sockFd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sockFd < 0) {
        MSPROF_LOGE("Dynamic profiling create server socket failed, sockFd=%d errno=%u.", sockFd, errno);
        return PROFILING_FAILED;
    }

    sockPath_ = Utils::IdeGetHomedir() + DYN_PROF_SOCK_UNIX_DOMAIN + std::to_string(MmGetPid());
    MSPROF_LOGI("Dynamic profiling server socket domain: %s.", sockPath_.c_str());
    sockaddr_un sockAddr;
    sockAddr.sun_family = AF_UNIX;
    errno_t err = strncpy_s(sockAddr.sun_path, sizeof(sockAddr.sun_path) - 1, sockPath_.c_str(), sockPath_.size());
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling server sockPath copy failed, err: %d, sockPath: %s", err, sockPath_.c_str());
        return PROFILING_FAILED;
    }

    MmUnlink(sockPath_);
    int ret = bind(sockFd, reinterpret_cast<struct sockaddr*>(&sockAddr), sizeof(sockAddr));
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling bind failed, ret=%d errno=%u.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    MmChmod(sockPath_, S_IRUSR | S_IWUSR);

    ret = listen(sockFd, 1);
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling listen failed, ret=%d errno=%u.", ret, errno);
        MmUnlink(sockPath_);
        close(sockFd);
        return PROFILING_FAILED;
    }

    srvSockFd_ = sockFd;
    return PROFILING_SUCCESS;
}

bool DyncProfMsgProcSrv::IdleConnectOverTime(uint32_t &recvIdleTimes) const
{
    if (profHasStarted_) {
        return false;
    }
    if (++recvIdleTimes <= DYN_PROF_IDLE_LINK_HOLD_TIME) {
        return false;
    }
    return true;
}

void DyncProfMsgProcSrv::DynProfServerProcMsg()
{
    std::string disconnTips;
    DynProfReqMsg recvMsg;
    uint32_t recvIdleTimes = 0;
    uint32_t recvMsgNum = 0;
    while (srvStarted_) {
        disconnTips = "Internal error.";
        ssize_t recvLen = recv(cliSockFd_, &recvMsg, sizeof(recvMsg), 0);
        if (static_cast<size_t>(recvLen) != sizeof(recvMsg) && errno == EAGAIN) {
            if (IdleConnectOverTime(recvIdleTimes)) {
                MSPROF_LOGW("Dynamic profiling disconnet client, recvIdleTimes=%u.", recvIdleTimes);
                disconnTips = "Idle link too long time.";
                break;
            }
            continue;
        } else if (static_cast<size_t>(recvLen) != sizeof(recvMsg)) {
            MSPROF_LOGE("Dynamic profiling process message recv fail, recvLen=%d errno=%u.", recvLen, errno);
            break;
        }
        if (recvMsg.msgDataLen != strnlen(recvMsg.msgData, DYN_PROF_REQ_MSG_MAX_LEN)) {
            MSPROF_LOGW("Dynamic profiling recv message error, type=%u len=%u.", recvMsg.msgType, recvMsg.msgDataLen);
            break;
        }
        MSPROF_LOGI("Dynamic profiling server receive new message, msgType=%u.", recvMsg.msgType);
        if (recvMsg.msgType == DynProfMsgType::TEST_REQ) {
            if (DynProfServerRsqMsg(DynProfMsgType::TEST_RSP, DynProfMsgProcRes::EXE_SUCC, "") != PROFILING_SUCCESS) {
                break;
            }
        } else if (recvMsg.msgType == DynProfMsgType::START_REQ) {
            recvParams_ = std::string(recvMsg.msgData, recvMsg.msgDataLen);
            recvIdleTimes = 0;
            DynProfSrvProcStart();
        } else if (recvMsg.msgType == DynProfMsgType::STOP_REQ) {
            DynProfSrvProcStop();
        } else if (recvMsg.msgType == DynProfMsgType::QUIT_REQ) {
            DynProfSrvProcQuit();
            disconnTips = "Client request quit.";
            break;
        } else {
            MSPROF_LOGE("Dynamic profiling process message receve unknow message, msgType=%u.", recvMsg.msgType);
            break;
        }
        if (++recvMsgNum > DYN_PROF_SERVER_PROC_MSG_MAX_NUM) {
            MSPROF_LOGW("Dynamic profiling receive message over %u.", recvMsgNum);
            disconnTips = "Receive too much requests from client.";
            break;
        }
    }
    NotifyClientDisconnet(disconnTips);
}

void DyncProfMsgProcSrv::DynProfSrvProcStart()
{
    if (profHasStarted_) {
        MSPROF_LOGW("Dynamic profiling repeat start.");
        DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_FAIL, "Collection has been started.");
        return;
    }

    if (DynProfMngSrv::instance()->startTimes_ > DYN_PROF_MAX_STARTABLE_TIMES) {
        MSPROF_LOGE("Dynamic profiling start over %d times.", DYN_PROF_MAX_STARTABLE_TIMES);
        DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_FAIL, "Start times over max limit.");
        return;
    }

    int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofInitAclEnv(recvParams_);
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling start MsprofInitAclEnv failed, ret=%d.", ret);
        DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_FAIL, "Internal error.");
        return;
    }

    if (Msprofiler::Api::ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling failed to init acl manager");
        DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_FAIL, "Internal error.");
        Msprofiler::Api::ProfAclMgr::instance()->SetModeToOff();
        return ;
    }

    std::string startDevId;
    std::string startSuccDevId;
    DynProfMngSrv::instance()->startTimes_ += 1;
    std::vector<ProfSetDevPara> devsInfo = DynProfMngSrv::instance()->GetDeviceInfo();
    for (auto &devInfo : devsInfo) {
        startDevId = startDevId + std::to_string(devInfo.deviceId) + " ";
        if (MsprofSetDeviceCallbackImpl(reinterpret_cast<VOID_PTR>(&devInfo), sizeof(devInfo)) != MSPROF_ERROR_NONE) {
            continue;
        }
        startSuccDevId += std::to_string(devInfo.deviceId) + " ";
    }
    std::string detailInfo = "Started device Id: " + startSuccDevId;
    if (startDevId != startSuccDevId || startDevId.empty()) {
        MSPROF_LOGE("Dynamic profiling start device failed, allDevId=%s, succDevId=%s.",
            startDevId.c_str(), startSuccDevId.c_str());
        DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_FAIL, detailInfo);
        DynProfMngSrv::instance()->startTimes_ -= 1;
        return;
    }

    profHasStarted_ = true;
    MSPROF_LOGI("Dynamic profiling start message process success, started device id: %s.", startSuccDevId.c_str());
    DynProfServerRsqMsg(DynProfMsgType::START_RSP, DynProfMsgProcRes::EXE_SUCC, detailInfo);
}

void DyncProfMsgProcSrv::DynProfSrvProcStop()
{
    if (!profHasStarted_) {
        MSPROF_LOGW("Dynamic profiling repeat stop.");
        DynProfServerRsqMsg(DynProfMsgType::STOP_RSP, DynProfMsgProcRes::EXE_FAIL, "Collection is not started.");
        return;
    }

    int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling stop MsprofFinalizeHandle failed, ret=%d.", ret);
        DynProfServerRsqMsg(DynProfMsgType::STOP_RSP, DynProfMsgProcRes::EXE_FAIL, "Internal error.");
        return;
    }

    profHasStarted_ = false;
    DynProfServerRsqMsg(DynProfMsgType::STOP_RSP, DynProfMsgProcRes::EXE_SUCC, "");
}

void DyncProfMsgProcSrv::DynProfSrvProcQuit()
{
    std::string detailInfo;
    DynProfMsgProcRes rsqCode = DynProfMsgProcRes::EXE_SUCC;

    if (profHasStarted_) {
        int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
        if (ret != MSPROF_ERROR_NONE) {
            MSPROF_LOGE("Dynamic profiling quit MsprofFinalizeHandle failed, ret=%d.", ret);
            rsqCode = DynProfMsgProcRes::EXE_FAIL;
            detailInfo = "Internal error.";
        }
    }
    DynProfServerRsqMsg(DynProfMsgType::QUIT_RSP, rsqCode, detailInfo);
    profHasStarted_ = false;
}

int DyncProfMsgProcSrv::DynProfServerRsqMsg(DynProfMsgType msgType, DynProfMsgProcRes rsqCode,
    const std::string &msgData)
{
    if (msgData.size() >= DYN_PROF_RSP_MSG_MAX_LEN) {
        MSPROF_LOGE("Dynamic profiling response message fail, msgData.size=%u.", msgData.size());
        return PROFILING_FAILED;
    }
    DynProfRspMsg rsqMsg;
    rsqMsg.msgType = msgType;
    rsqMsg.statusCode = rsqCode;
    rsqMsg.msgDataLen = msgData.size();
    errno_t err = memcpy_s(rsqMsg.msgData, sizeof(rsqMsg.msgData) - 1, msgData.c_str(), msgData.size());
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling server copy rsqMsg failed, err: %d, rsqMsg: %s", err, msgData.c_str());
        return PROFILING_FAILED;
    }
    ssize_t sendLen = send(cliSockFd_, &rsqMsg, sizeof(rsqMsg), MSG_NOSIGNAL);
    if (static_cast<size_t>(sendLen) != sizeof(rsqMsg)) {
        MSPROF_LOGW("Dynamic profiling response message fail, msgType=%u statusCode=%d sendLen=%d errno=%u.",
            msgType, rsqCode, sendLen, errno);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void DyncProfMsgProcSrv::NotifyClientDisconnet(const std::string &detailInfo)
{
    (void)DynProfServerRsqMsg(DynProfMsgType::DISCONN_RSP, DynProfMsgProcRes::EXE_SUCC, detailInfo);
    DynProfSrvProcQuit();
}

DynProfMngSrv::DynProfMngSrv() : startTimes_(0), started_(false)
{
}

DynProfMngSrv::~DynProfMngSrv()
{
}

int DynProfMngSrv::StartDynProfSrv()
{
    if (started_) {
        MSPROF_LOGI("Dynamic profiling server has been started.");
        return PROFILING_SUCCESS;
    }
    MSVP_MAKE_SHARED0_RET(dynProfSrv_, DyncProfMsgProcSrv, PROFILING_FAILED);
    if (dynProfSrv_->Start() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling start server fail.");
        return PROFILING_FAILED;
    }
    started_ = true;
    MSPROF_LOGI("Dynamic profiling start server success.");
    return PROFILING_SUCCESS;
}

void DynProfMngSrv::StopDynProfSrv()
{
    if (!started_) {
        MSPROF_LOGI("Dynamic profiling server has been stoped.");
        return;
    }
    // notify client
    dynProfSrv_->NotifyClientDisconnet("Server process exit.");
    if (dynProfSrv_ != nullptr) {
        (void)dynProfSrv_->Stop();
    }
    started_ = false;
    MSPROF_LOGI("Dynamic profiling stop server.");
}

void DynProfMngSrv::SetDeviceInfo(ProfSetDevPara *data)
{
    devicesInfo_.push_back(*data);
}

std::vector<ProfSetDevPara> &DynProfMngSrv::GetDeviceInfo()
{
    return devicesInfo_;
}

bool DynProfMngSrv::IsStarted()
{
    return started_;
}
}
}
}