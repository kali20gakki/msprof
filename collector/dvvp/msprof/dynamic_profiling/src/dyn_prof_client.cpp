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
    std::string echoTips;

    std::cout << INTERACTION_MODE_PREFIX << std::flush;
    while (cliStarted_) {
        if (IsServerDisconnect(echoTips)) {
            std::cout << std::endl << echoTips << std::endl;
            break;
        }
        if (TryReadInputCmd(inputCmd) < 0) {
            continue;
        }
        if (inputCmd.empty()) {
            std::cout << INTERACTION_MODE_PREFIX << std::flush;
            continue;
        }
        if (inputCmd == INTERACTION_MODE_CMD_START) {
            (void)SendMsgToServer(DynProfMsgType::START_REQ, DynProfMsgType::START_RSP, sendParams_, echoTips);
        } else if (inputCmd == INTERACTION_MODE_CMD_STOP) {
            (void)SendMsgToServer(DynProfMsgType::STOP_REQ, DynProfMsgType::STOP_RSP, "", echoTips);
        } else if (inputCmd == INTERACTION_MODE_CMD_QUIT) {
            (void)SendMsgToServer(DynProfMsgType::QUIT_REQ, DynProfMsgType::QUIT_RSP, "", echoTips);
        } else {
            echoTips = DynProfCliProcHelp();
        }
        std::cout << echoTips << std::endl;
        std::cout << INTERACTION_MODE_PREFIX  << std::flush;
    }
}

int DyncProfMsgProcCli::TryReadInputCmd(std::string &inputCmd)
{
    timeval timeout = {DYN_PROF_READ_INPUT_CMD_WAIT_TIME, 0};
    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(0, &fdSet);
    if (select(1, &fdSet, nullptr, nullptr, &timeout) == 0) {
        return -1;
    }
    (void)getline(std::cin, inputCmd);
    inputCmd = Utils::Trim(inputCmd);
    return inputCmd.size();
}

bool DyncProfMsgProcCli::IsServerDisconnect(std::string &echoTips)
{
    DynProfRspMsg rspMsg;
    ssize_t recvLen = recv(cliSockFd_, reinterpret_cast<VOID_PTR>(&rspMsg), sizeof(rspMsg), MSG_DONTWAIT);
    if (static_cast<size_t>(recvLen) == sizeof(rspMsg) &&
        rspMsg.msgType == DynProfMsgType::DISCONN_RSP &&
        rspMsg.msgDataLen < DYN_PROF_RSP_MSG_MAX_LEN) {
        echoTips = "Server disconnected, " + std::string(rspMsg.msgData, rspMsg.msgDataLen);
        MSPROF_LOGI("Dynamic profiling client receive disconnet from server.");
        return true;
    }
    return false;
}

int DyncProfMsgProcCli::SetParams(const std::string &params)
{
    if (params.size() >= DYN_PROF_REQ_MSG_MAX_LEN || params.size() == 0) {
        MSPROF_LOGE("Dynamic profiling param length error, len=%d.", params.size());
        return PROFILING_FAILED;
    }
    sendParams_ = params;
    return PROFILING_SUCCESS;
}

std::string DyncProfMsgProcCli::DynProfCliProcHelp()
{
    std::string helpInfo = "Usage:\n"
        "\t" + INTERACTION_MODE_CMD_START + ":\tStart a collection in interactive mode.\n"
        "\t" + INTERACTION_MODE_CMD_STOP + ":\tStop a collection that was started in interactive mode.\n"
        "\t" + INTERACTION_MODE_CMD_QUIT + ":\tStop collection and quit interactive mode.";
    return helpInfo;
}

int DyncProfMsgProcCli::ConnectSocket(int sockFd, struct sockaddr *serv_addr, int addrlen) const
{
    int ret = fcntl(sockFd, F_GETFL, 0);
    if (ret < 0) {
        MSPROF_LOGE("Dynamic profiling client get sockFd fail, ret=%d, errno=%u.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    if (fcntl(sockFd, F_SETFL, static_cast<unsigned int>(ret) | O_NONBLOCK) < 0) {
        MSPROF_LOGE("Dynamic profiling client set sockFd NONBLOCK fail, ret=%d, errno=%u.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    ret = connect(sockFd, serv_addr, addrlen);
    if (ret == -1) {
        MSPROF_LOGE("Dynamic profiling client connect fail, ret=%d errno=%u.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    ret = fcntl(sockFd, F_GETFL, 0);
    if (ret < 0) {
        MSPROF_LOGE("Dynamic profiling client get sockFd fail, ret=%d, errno=%u.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    if (fcntl(sockFd, F_SETFL, static_cast<unsigned int>(ret) & (~O_NONBLOCK)) < 0) {
        MSPROF_LOGE("Dynamic profiling client set sockFd BLOCK fail, ret=%d, errno=%u.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int DyncProfMsgProcCli::SetSocketMsgTimeout(int sockFd) const
{
    timeval timeout1 = {DYN_PROF_CLIENT_SEND_WAIT_TIME, 0};
    int ret = setsockopt(sockFd,
                         SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<CHAR_PTR>(&timeout1), sizeof(struct timeval));
    if (ret < 0) {
        MSPROF_LOGE("Dynamic profiling client set sockFd send data timeout fail, ret=%d, errno=%u.", ret, errno);
        return PROFILING_FAILED;
    }
    timeval timeout2 = {DYN_PROF_CLIENT_RECV_WAIT_TIME, 0};
    ret = setsockopt(sockFd,
                     SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<CHAR_PTR>(&timeout2), sizeof(struct timeval));
    if (ret < 0) {
        MSPROF_LOGE("Dynamic profiling client set sockFd receive data timeout fail, ret=%d, errno=%u.", ret, errno);
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int DyncProfMsgProcCli::CreateDynProfClientSock()
{
    // create socket file
    int sockFd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sockFd < 0) {
        MSPROF_LOGE("Dynamic profiling create client socket fail, sockFd=%d errno=%u.", sockFd, errno);
        return PROFILING_FAILED;
    }
    // socket address
    std::string sockPath = Utils::IdeGetHomedir() + DYN_PROF_SOCK_UNIX_DOMAIN +
        std::to_string(DynProfMngCli::instance()->GetAppPid());
    MSPROF_LOGI("Dynamic profiling client socket domain: %s.", sockPath.c_str());
    sockaddr_un sockAddr;
    sockAddr.sun_family = AF_UNIX;
    errno_t err = strncpy_s(sockAddr.sun_path, sizeof(sockAddr.sun_path) - 1, sockPath.c_str(), sockPath.size());
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling client sockPath copy failed, err: %d, sockPath: %s", err, sockPath.c_str());
        return PROFILING_FAILED;
    }
    // connect socket
    if (ConnectSocket(sockFd, reinterpret_cast<sockaddr *>(&sockAddr), sizeof(sockAddr)) == PROFILING_FAILED) {
        return PROFILING_FAILED;
    }
    // check receive/send
    cliSockFd_ = sockFd;
    std::string echoTips;
    timeval timeout = {0, DYN_PROF_CLIENT_CONNECT_WAIT_TIME};
    int ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<CHAR_PTR>(&timeout), sizeof(struct timeval));
    if (ret < 0) {
        MSPROF_LOGE("Dynamic profiling client set sockFd receive data timeout fail, ret=%d, errno=%u.", ret, errno);
        close(sockFd);
        return PROFILING_FAILED;
    }
    ret = SendMsgToServer(DynProfMsgType::TEST_REQ, DynProfMsgType::TEST_RSP, "", echoTips);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Dynamic profiling client send message to server fail.");
        close(sockFd);
        cliSockFd_ = -1;
        return PROFILING_FAILED;
    }
    // set socket send/receive message wait time
    if (SetSocketMsgTimeout(sockFd) == PROFILING_FAILED) {
        close(sockFd);
        cliSockFd_ = -1;
        return PROFILING_FAILED;
    }
    // client socket create success
    MSPROF_LOGI("Dynamic profiling client connet server success.");
    cliSockFd_ = sockFd;
    return PROFILING_SUCCESS;
}

int DyncProfMsgProcCli::SendMsgToServer(DynProfMsgType reqMsgtype, DynProfMsgType rspMsgtype,
    const std::string &reqMsgParams, std::string &echoTips)
{
    echoTips = "Execution fail, Internal error.";

    // send message
    DynProfReqMsg reqMsg;
    reqMsg.msgType = reqMsgtype;
    reqMsg.msgDataLen = reqMsgParams.size();
    errno_t err = memset_s(reqMsg.msgData, sizeof(reqMsg.msgData), 0, sizeof(reqMsg.msgData));
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling client clear reqMsg failed, err: %d", err);
        return PROFILING_FAILED;
    }
    err = memcpy_s(reqMsg.msgData, sizeof(reqMsg.msgData) - 1, reqMsgParams.c_str(), reqMsgParams.size());
    if (err != EOK) {
        MSPROF_LOGE("Dynamic profiling client copy reqMsg failed, err: %d, reqMsg: %s", err, reqMsgParams.c_str());
        return PROFILING_FAILED;
    }
    ssize_t sendLen = send(cliSockFd_, reinterpret_cast<VOID_PTR>(&reqMsg), sizeof(reqMsg), MSG_NOSIGNAL);
    if (static_cast<size_t>(sendLen) != sizeof(reqMsg)) {
        MSPROF_LOGE("cliSockFd_=%d send msg fail, sendLen=%d, errno=%u", cliSockFd_, sendLen, errno);
        return PROFILING_FAILED;
    }
    // receive message
    DynProfRspMsg rspMsg;
    ssize_t recvLen = recv(cliSockFd_, reinterpret_cast<VOID_PTR>(&rspMsg), sizeof(rspMsg), 0);
    if (static_cast<size_t>(recvLen) != sizeof(rspMsg)) {
        MSPROF_LOGE("cliSockFd_=%d recv msg fail, recvLen=%d, errno=%u", cliSockFd_, recvLen, errno);
        return PROFILING_FAILED;
    }
    if (rspMsg.msgType != rspMsgtype ||
        rspMsg.statusCode > DynProfMsgProcRes::EXE_FAIL ||
        rspMsg.msgDataLen >= DYN_PROF_RSP_MSG_MAX_LEN) {
        MSPROF_LOGE("server msg error, rspMsgtype=%d statusCode=%d msgDataLen=%u",
                    rspMsg.msgType, rspMsg.statusCode, rspMsg.msgDataLen);
        return PROFILING_FAILED;
    }

    // encapsulate tips.
    std::string detail = (rspMsg.msgDataLen == 0) ? (".") : (", " + std::string(rspMsg.msgData, rspMsg.msgDataLen));
    if (rspMsg.statusCode != DynProfMsgProcRes::EXE_SUCC) {
        echoTips = "Execution fail" + detail;
        return PROFILING_FAILED;
    } else {
        echoTips = "Execution success" + detail;
        return PROFILING_SUCCESS;
    }
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

int DynProfMngCli::GetRealAppPid(int pid)
{
    // get all dynamic profiling server pid
    CHAR_PTR end = nullptr;
    constexpr int base = 10;
    std::vector<int> sockPids;
    std::vector<std::string> fileNameList;
    std::string homeDir = Utils::IdeGetHomedir();
    Utils::GetChildFilenames(homeDir, fileNameList);
    for (auto fileName : fileNameList) {
        std::string filePath = homeDir + MSVP_SLASH + fileName;
        if (!Utils::IsSocketFile(filePath)) {
            continue;
        }
        size_t pos = filePath.find(DYN_PROF_SOCK_UNIX_DOMAIN);
        if (pos == std::string::npos) {
            continue;
        }
        std::string pidStr = filePath.substr(pos + DYN_PROF_SOCK_UNIX_DOMAIN.size());
        int pid = std::strtol(pidStr.c_str(), &end, base);
        sockPids.push_back(pid);
    }

    if (sockPids.empty())
        MSPROF_LOGI("sockPids is empty");

    // get current process child pid
    std::vector<int> childPids = Utils::GetChildPidRecursive(pid, 0);
    childPids.insert(childPids.begin(), pid);

    // get intersection pid
    std::vector<int> appPids;
    for (auto childPid : childPids) {
        for (auto sockPid : sockPids) {
            if (childPid == sockPid) {
                appPids.push_back(childPid);
            }
        }
    }
    if (appPids.empty()) {
        MSPROF_LOGI("appPids is empty");
        return 0;
    }
    MSPROF_LOGI("appPids is %d", appPids[0]);
    return appPids[0];
}

int DynProfMngCli::TryGetRealAppPid(int pid)
{
    static const int tryTimes = 20;
    static const int waitTimeMs = 1000;
    int readAppPid = 0;
    for (int i = 0; i < tryTimes; i++) {
        Utils::MsleepInterruptible(waitTimeMs);
        readAppPid = GetRealAppPid(pid);
        if (readAppPid > 0) {
            return readAppPid;
        }
        MSPROF_LOGD("Dynamic profiling GetRealAppPid try %d times.", i + 1);
    }
    MSPROF_LOGW("Dynamic profiling get server socket pid fail.");
    return 0;
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