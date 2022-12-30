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
    srvStarted_ = false;
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
    sockaddr_un clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    while (srvStarted_) {
        cliSockFd_ = accept(srvSockFd_, (struct sockaddr*)&clt_addr, &clt_addr_len);
        if (cliSockFd_ < 0) {
            if (errno == EAGAIN) {
                continue;   // timeout
            } else {
                MSPROF_LOGE("Dynamic profiling accept failed, cliSockFd_=%d errno=%d.", cliSockFd_, errno);
                break;
            }
        }
        DynProfServerProcMsg();
    }
}
 
int DyncProfMsgProcSrv::DynProfServerCreateSock()
{
    std::string sockPath = DYN_PROF_SOCK_UNIX_DOMAIN + std::to_string(MmGetPid());
    MSPROF_LOGI("Dynamic profiling server socket domain: %s.", sockPath.c_str());
    
    int sockFd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sockFd < 0) {
        MSPROF_LOGE("Dynamic profiling create server socket failed, sockFd=%d errno=%d.", sockFd, errno);
        return PROFILING_FAILED;
    }
 
    sockaddr_un sockAddr;
    errno_t err = memset_s(&sockAddr, sizeof(sockAddr), 0, sizeof(sockAddr));
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling memset_s failed, err=%d.", err);
        close(sockFd);
        return PROFILING_FAILED;
    }
    err = memcpy_s(sockAddr.sun_path + 1, sizeof(sockAddr.sun_path) - 1, sockPath.c_str(), sockPath.size());
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling memcpy_s failed, err=%d.", err);
        close(sockFd);
        return PROFILING_FAILED;
    }
 
    sockAddr.sun_family = AF_LOCAL;
    int ret = bind(sockFd, reinterpret_cast<struct sockaddr*>(&sockAddr),
        offsetof(sockaddr_un, sun_path) + 1 + sockPath.size()) ;
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling bind failed, ret=%d errno=%d.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
 
    ret = listen(sockFd, 1);
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling listen failed, ret=%d errno=%d.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
 
    timeval timeout = {1, 0};   // accept socket wait 1 second
    ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<CHAR_PTR>(&timeout), sizeof(struct timeval));
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling setsockopt failed, ret=%d errno=%d.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
 
    srvSockFd_ = sockFd;
    return PROFILING_SUCCESS;
}
 
void DyncProfMsgProcSrv::DynProfServerProcMsg()
{
    timeval timeout = {1, 0};    // read socket wait 1 second
    int ret = setsockopt(cliSockFd_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<CHAR_PTR>(&timeout),
        sizeof(struct timeval));
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling setsockopt failed, ret=%d errno=%d.", ret, errno);
        close(cliSockFd_);
        return;
    }
 
    DynProfMsg recvMsg;
    while (srvStarted_) {
        ssize_t readLen = read(cliSockFd_, &recvMsg, sizeof(recvMsg));
        if (readLen < 0) {
            if (errno == EAGAIN) {
                continue;   // timeout
            } else {
                MSPROF_LOGE("Dynamic profiling process message read fail, readLen=%d errno=%d.", readLen, errno);
                break;
            }
        }
        if (recvMsg.msgDataLen >= DYN_PROF_PARAMS_MAX_LEN) {
            MSPROF_LOGE("Dynamic profiling recv message length over %d, msgType=%d msgDataLen=%d.",
                DYN_PROF_PARAMS_MAX_LEN, recvMsg.msgType, recvMsg.msgDataLen);
            break;
        }
        recvParams_ = std::string(recvMsg.msgData, recvMsg.msgDataLen);
        if (recvMsg.msgType == DynProfMsgType::START_REQ) {
            DynProfSrvProcStart();
        } else if (recvMsg.msgType == DynProfMsgType::STOP_REQ) {
            DynProfSrvProcStop();
        } else if (recvMsg.msgType == DynProfMsgType::QUIT_REQ) {
            DynProfSrvProcQuit();
            break;
        } else {
            MSPROF_LOGE("Dynamic profiling process message receve unknow message, msgType=%d.", recvMsg.msgType);
            DynProfServerRsqMsg(DynProfMsgType::QUIT_RSQ, DynProfMsgRsqCode::RSQ_FAIL);
        }
    }
}
 
void DyncProfMsgProcSrv::DynProfSrvProcStart()
{
    if (profHasStarted_) {
        MSPROF_LOGW("Dynamic profiling repeat start.");
        DynProfServerRsqMsg(DynProfMsgType::START_RSQ, DynProfMsgRsqCode::RSQ_FAIL);
        return;
    }
 
    int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofInitAclEnv(recvParams_);
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling start MsprofInitAclEnv failed, ret=%d.", ret);
        DynProfServerRsqMsg(DynProfMsgType::START_RSQ, DynProfMsgRsqCode::RSQ_FAIL);
        return;
    }
 
    if (Msprofiler::Api::ProfAclMgr::instance()->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling failed to init acl manager");
        DynProfServerRsqMsg(DynProfMsgType::START_RSQ, DynProfMsgRsqCode::RSQ_FAIL);
        Msprofiler::Api::ProfAclMgr::instance()->SetModeToOff();
        return ;
    }
 
    DynProfMngSrv::instance()->startTimes_ += 1;
    ProfSetDevPara *devInfo = DynProfMngSrv::instance()->GetDeviceInfo();
    ret = MsprofSetDeviceCallbackImpl(reinterpret_cast<VOID_PTR>(devInfo), sizeof(ProfSetDevPara));
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling start MsprofSetDeviceCallbackImpl failed, ret=%d.", ret);
        DynProfServerRsqMsg(DynProfMsgType::START_RSQ, DynProfMsgRsqCode::RSQ_FAIL);
        DynProfMngSrv::instance()->startTimes_ -= 1;
        return;
    }
 
    profHasStarted_ = true;
    MSPROF_LOGI("Dynamic profiling start message process success.");
    DynProfServerRsqMsg(DynProfMsgType::START_RSQ, DynProfMsgRsqCode::RSQ_SUCCESS);
}
 
void DyncProfMsgProcSrv::DynProfSrvProcStop()
{
    if (!profHasStarted_) {
        MSPROF_LOGW("Dynamic profiling repeat stop.");
        DynProfServerRsqMsg(DynProfMsgType::STOP_RSQ, DynProfMsgRsqCode::RSQ_FAIL);
        return;
    }
 
    int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
    if (ret != MSPROF_ERROR_NONE) {
        MSPROF_LOGE("Dynamic profiling stop MsprofFinalizeHandle failed, ret=%d.", ret);
        DynProfServerRsqMsg(DynProfMsgType::STOP_RSQ, DynProfMsgRsqCode::RSQ_FAIL);
        return;
    }
 
    profHasStarted_ = false;
    DynProfServerRsqMsg(DynProfMsgType::STOP_RSQ, DynProfMsgRsqCode::RSQ_SUCCESS);
}
 
void DyncProfMsgProcSrv::DynProfSrvProcQuit()
{
    DynProfMsgRsqCode rsqCode = DynProfMsgRsqCode::RSQ_SUCCESS;
 
    if (profHasStarted_) {
        int ret = Msprofiler::Api::ProfAclMgr::instance()->MsprofFinalizeHandle();
        if (ret != MSPROF_ERROR_NONE) {
            MSPROF_LOGE("Dynamic profiling quit MsprofFinalizeHandle failed, ret=%d.", ret);
            rsqCode = DynProfMsgRsqCode::RSQ_FAIL;
        }
    }
    DynProfServerRsqMsg(DynProfMsgType::QUIT_RSQ, rsqCode);
    profHasStarted_ = false;
}
 
void DyncProfMsgProcSrv::DynProfServerRsqMsg(DynProfMsgType msgType, DynProfMsgRsqCode rsqCode)
{
    DynProfMsg rsqMsg;
    rsqMsg.msgType = msgType;
    rsqMsg.statusCode = rsqCode;
    ssize_t writeLen = write(cliSockFd_, &rsqMsg, sizeof(rsqMsg));
    if (writeLen != sizeof(rsqMsg)) {
        MSPROF_LOGW("Dynamic profiling response message fail, msgType=%d statusCode=%d writeLen=%d.",
            msgType, rsqCode, writeLen);
    }
}
 
DynProfMngSrv::DynProfMngSrv() : startTimes_(0)
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
    if (dynProfSrv_ != nullptr) {
        (void)dynProfSrv_->Stop();
    }
    started_ = false;
    MSPROF_LOGI("Dynamic profiling stop server.");
}
 
void DynProfMngSrv::SetDeviceInfo(ProfSetDevPara *data)
{
    deviceInfo_ = *data;
}
 
ProfSetDevPara *DynProfMngSrv::GetDeviceInfo()
{
    return &deviceInfo_;
}
 
bool DynProfMngSrv::IsStarted()
{
    return started_;
}
}
}
}