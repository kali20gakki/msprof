/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: dynamic profiling manager
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2022-12-10
 */
#include "dyn_prof_client.h"
 
#include <algorithm>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
 
#include "config/config.h"
#include "errno/error_code.h"
#include "mmpa_api.h"
#include "msprof_dlog.h"
#include "utils/utils.h"
 
namespace Collector {
namespace Dvvp {
namespace DynProf {
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::MsprofErrMgr;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Mmpa;
using namespace analysis::dvvp::common::config;
 
DyncProfMsgProcCli::DyncProfMsgProcCli() : cliStarted_(false), cliSockFd_(-1)
{
}
 
DyncProfMsgProcCli::~DyncProfMsgProcCli()
{
}
 
int DyncProfMsgProcCli::Start()
{
    MSPROF_LOGI("Dynamic profiling begin to init client.");
    if (cliStarted_) {
        MSPROF_LOGW("Dynamic profiling client thread has been started.");
        return PROFILING_SUCCESS;
    }
    if (CreateDynProfClientSock() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling create client socket fail.");
        return PROFILING_FAILED;
    }
    cliStarted_ = true;
    Thread::SetThreadName(analysis::dvvp::common::config::MSVP_DYN_PROF_CLIENT_THREAD_NAME);
    int ret = Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        Stop();
        MSPROF_LOGE("Dynamic profiling start thread fail.");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Dynamic profiling init client success.");
    return PROFILING_SUCCESS;
}
 
int DyncProfMsgProcCli::Stop()
{
    MSPROF_LOGI("Dynamic profiling begin to stop client.");
    if (!cliStarted_) {
        MSPROF_LOGW("Dynamic profiling client not started.");
        return PROFILING_SUCCESS;
    }
    cliStarted_ = false;
    if (cliSockFd_ > 0) {
        close(cliSockFd_);
        cliSockFd_ = -1;
    }
    MSPROF_LOGI("Dynamic profiling stop client success.");
    return PROFILING_SUCCESS;
}
 
void DyncProfMsgProcCli::Run(const struct error_message::Context &errorContext)
{
    MSPROF_LOGI("Dynamic profiling client run entry.");
    std::string inputCmd;
    std::string echoCmd;
 
    while (cliStarted_) {
        std::cout << "> ";
        (void)getline(std::cin, inputCmd);
        switch (ParserInputCmd(inputCmd)) {
            case DynProfCliCmd::CMD_START:
                echoCmd = DynProfCliProcStart();
                break;
            case DynProfCliCmd::CMD_STOP:
                echoCmd = DynProfCliProcStop();
                break;
            case DynProfCliCmd::CMD_QUIT:
                echoCmd = DynProfCliProcQuit();
                break;
            case DynProfCliCmd::CMD_HELP:
                echoCmd = "start: Start a collection in interactive mode.\n"
                    "stop: Stop a collection that was started in interactive mode.\n"
                    "quit: Stop collection and quit interactive mode.";
                break;
            case DynProfCliCmd::CMD_UNKNOW:
                echoCmd = "unknown command, input 'help' for more information.";
                break;
            case DynProfCliCmd::CMD_EMPTY:
            default:
                echoCmd = "";
                break;
        }
        if (!echoCmd.empty()) {
            std::cout << echoCmd << std::endl;
        }
    }
}
 
int DyncProfMsgProcCli::SetParams(const std::string &params)
{
    if (params.size() >= DYN_PROF_PARAMS_MAX_LEN || params.size() == 0) {
        MSPROF_LOGE("Dynamic profiling param length error, len=%d.", params.size());
        return PROFILING_FAILED;
    }
    sendParams_ = params;
    return PROFILING_SUCCESS;
}
 
std::string DyncProfMsgProcCli::DynProfCliProcStart()
{
    std::string cmdLineEcho;
    int ret = SendMsgToServer(DynProfMsgType::START_REQ, DynProfMsgType::START_RSQ, sendParams_);
    if (ret == PROFILING_FAILED) {
        cmdLineEcho = "Start collection failed.";
        MSPROF_LOGE("Dynamic profiling client execute start cmd fail.");
    } else {
        MSPROF_LOGI("Dynamic profiling client execute start cmd success.");
    }
    return cmdLineEcho;
}
 
std::string DyncProfMsgProcCli::DynProfCliProcStop()
{
    std::string cmdLineEcho;
    int ret = SendMsgToServer(DynProfMsgType::STOP_REQ, DynProfMsgType::STOP_RSQ, "");
    if (ret == PROFILING_FAILED) {
        cmdLineEcho = "Stop collection failed.";
        MSPROF_LOGE("Dynamic profiling client execute stop cmd fail.");
    } else {
        MSPROF_LOGI("Dynamic profiling client execute stop cmd success.");
    }
    return cmdLineEcho;
}
 
std::string DyncProfMsgProcCli::DynProfCliProcQuit()
{
    std::string cmdLineEcho;
    int ret = SendMsgToServer(DynProfMsgType::QUIT_REQ, DynProfMsgType::QUIT_RSQ, "");
    if (ret == PROFILING_FAILED) {
        cmdLineEcho = "Quit failed.";
        MSPROF_LOGE("Dynamic profiling client execute quit cmd fail.");
    } else {
        MSPROF_LOGI("Dynamic profiling client execute quit cmd success.");
    }
    cliStarted_ = false;
    DynProfMngCli::instance()->StopDynProfCli();
    return cmdLineEcho;
}
 
DynProfCliCmd DyncProfMsgProcCli::ParserInputCmd(const std::string &inputCmd)
{
    std::string stripCmd = inputCmd;
    if (stripCmd.empty()) {
        return DynProfCliCmd::CMD_EMPTY;
    } else if (stripCmd == "start") {
        return DynProfCliCmd::CMD_START;
    } else if (stripCmd == "stop") {
        return DynProfCliCmd::CMD_STOP;
    } else if (stripCmd == "quit") {
        return DynProfCliCmd::CMD_QUIT;
    } else if (stripCmd == "help") {
        return DynProfCliCmd::CMD_HELP;
    } else {
        return DynProfCliCmd::CMD_UNKNOW;
    }
}
 
int DyncProfMsgProcCli::CreateDynProfClientSock()
{
    std::string sockPath = DYN_PROF_SOCK_UNIX_DOMAIN + std::to_string(DynProfMngCli::instance()->GetAppPid());
    MSPROF_LOGI("Dynamic profiling client socket domain: %s.", sockPath.c_str());
 
    int sockFd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sockFd < 0) {
        MSPROF_LOGE("Dynamic profiling create client socket fail, sockFd=%d errno=%d.", sockFd, errno);
        return PROFILING_FAILED;
    }
 
    sockaddr_un sockAddr;
    (void)memset_s(&sockAddr, sizeof(sockAddr), 0, sizeof(sockAddr));
    sockAddr.sun_family = AF_LOCAL;
    errno_t err = strncpy_s(sockAddr.sun_path + 1, sizeof(sockAddr.sun_path) - 1,
        sockPath.c_str(), sockPath.size());
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling strncpy_s fail, err=%d.", err);
        close(sockFd);
        return PROFILING_FAILED;
    }
 
    int ret = connect(sockFd, (struct sockaddr*)&sockAddr,
        offsetof(struct sockaddr_un, sun_path) + 1 + sockPath.size());
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Dynamic profiling client connect fail, ret=%d errno=%d.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
 
    timeval timeout = {1, 0};
    ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<CHAR_PTR>(&timeout), sizeof(struct timeval));
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling setsockopt fail, ret=%d errno=%d.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<CHAR_PTR>(&timeout), sizeof(struct timeval));
    if (ret != 0) {
        MSPROF_LOGE("Dynamic profiling setsockopt fail, ret=%d errno=%d.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
 
    MSPROF_LOGI("Dynamic profiling client connet server success.");
    cliSockFd_ = sockFd;
    return PROFILING_SUCCESS;
}
 
int DyncProfMsgProcCli::SendMsgToServer(DynProfMsgType reqMsgtype, DynProfMsgType rsqMsgtype,
    const std::string &reqMsgParams)
{
    DynProfMsg msg;
    msg.msgType = reqMsgtype;
    msg.msgDataLen = reqMsgParams.size();
    (void)memcpy_s(msg.msgData, sizeof(msg.msgData) - 1, reqMsgParams.c_str(), reqMsgParams.size());
 
    ssize_t sendLen = write(cliSockFd_, reinterpret_cast<VOID_PTR>(&msg), sizeof(msg));
    if (sendLen != sizeof(msg)) {
        MSPROF_LOGE("cliSockFd_=%d write msg fail, magLen=%llu, sendLen=%d, errno=%d",
            cliSockFd_, sizeof(msg), sendLen, errno);
        return PROFILING_FAILED;
    }
    ssize_t recvLen = read(cliSockFd_, reinterpret_cast<VOID_PTR>(&msg), sizeof(msg));
    if (recvLen != sizeof(msg)) {
        MSPROF_LOGE("cliSockFd_=%d read msg fail, magLen=%llu, recvLen=%d, errno=%d",
            cliSockFd_, sizeof(msg), recvLen, errno);
        return PROFILING_FAILED;
    }
    if (msg.msgType != rsqMsgtype || msg.statusCode != DynProfMsgRsqCode::RSQ_SUCCESS) {
        MSPROF_LOGE("server msg process fail, rsqMsgtype=%d statusCode=%d", msg.msgType, msg.statusCode);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
 
DynProfMngCli::DynProfMngCli() : enabled_(false), appPid_(0)
{
}
 
DynProfMngCli::~DynProfMngCli()
{
}
 
int DynProfMngCli::StartDynProfCli(const std::string &params)
{
    MSVP_MAKE_SHARED0_RET(dynProfCli_, DyncProfMsgProcCli, PROFILING_FAILED);
    if (dynProfCli_->SetParams(params) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling set params fail.");
        return PROFILING_FAILED;
    }
    if (dynProfCli_->Start() != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling start client thread fail.");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Dynamic profiling start client.");
    return PROFILING_SUCCESS;
}
 
void DynProfMngCli::StopDynProfCli()
{
    if (dynProfCli_ != nullptr) {
        (void)dynProfCli_->Stop();
    }
    MSPROF_LOGI("Dynamic profiling stop client.");
}
 
void DynProfMngCli::SetAppPid(int pid)
{
    appPid_ = pid;
}
 
int DynProfMngCli::GetAppPid()
{
    return appPid_;
}
 
void DynProfMngCli::EnableMode()
{
    enabled_ = true;
}
 
bool DynProfMngCli::IsEnableMode()
{
    return enabled_;
}
 
std::string DynProfMngCli::ConstructEnv()
{
    if (enabled_) {
        return PROFILING_MODE_ENV + "=" + DAYNAMIC_PROFILING_VALUE;
    } else {
        return "";
    }
}
 
void DynProfMngCli::WaitQuit()
{
    if (dynProfCli_ != nullptr) {
        dynProfCli_->Join();
    }
}
}
}
}