/**
 * @file ide_daemon.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "common/config.h"
#include "ide_common_util.h"
#include "ide_task_register.h"
#include "config/adx_config_manager.h"
#include "stackcore/stackcore.h"

#ifdef IAM
#include "iam.h"
#endif

#if (OS_TYPE == WIN)
#include <thread>
#endif

using namespace IdeDaemon::Common::Config;
using namespace Adx;

/**
 * @berif : start single Process
 * @param : none
 * @return: open file fd
 */
int SingleProcessStart(std::string &lockInfo)
{
    const int fileMaskWc = 0600;
    lockInfo = Adx::Manager::Config::AdxConfigManager::Instance().GetAdtmpPath();
    lockInfo.append(".");
    lockInfo.append(IDE_DAEMON_NAME);
    lockInfo.append(".lock");
    int fd = mmOpen2(lockInfo.c_str(), O_WRONLY | O_CREAT, fileMaskWc);
    if (fd < 0) {
        IDE_LOGE("single process open %s fd = %d", lockInfo.c_str(), fd);
        return IDE_DAEMON_ERROR;
    }

    struct flock lock;
    (void)memset_s(&lock, sizeof(lock), 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    int ret = IdeLockFcntl(fd, F_SETLK, lock);
    if (ret < 0) {
        IDE_LOGE("ada is exist, don't start again");
        printf("ada is exist, don't start again\n");
        IDE_MMCLOSE_AND_SET_INVALID(fd);
        // other process has locked the file, must not remove the file
        return IDE_DAEMON_ERROR;
    }

    int val = IdeFcntl(fd, F_GETFD, 0);
    if (val < 0) {
        IDE_LOGE("single process fcntl get fd");
        IDE_MMCLOSE_AND_SET_INVALID(fd);
        return IDE_DAEMON_ERROR;
    }

    auto flag = static_cast<unsigned int>(val);
    flag |= FD_CLOEXEC;
    if (IdeFcntl(fd, F_SETFD, flag) < 0) {
        IDE_LOGE("single process fcntl set fd");
        IDE_MMCLOSE_AND_SET_INVALID(fd);
        return IDE_DAEMON_ERROR;
    }
    return fd;
}

int AdxStartUpInit()
{
    // register daemon modules
    IdeDaemonRegisterModules();
    // DaemonInit function
    if (DaemonInit() != IDE_DAEMON_OK) {
        IDE_LOGE("ada init failed, exit");
        printf("ada init failed, exit\n");
        return IDE_DAEMON_ERROR;
    }
    // handle request
    HdcCreateHdcServerProc(nullptr);
    return IDE_DAEMON_OK;
}

/**
 * @berif : start ide daemon
 * @param : [in]isFork: if fork child process or not
 * @return: IDE_DAEMON_ERROR(-1) : failed
 *          IDE_DAEMON_ERROR(0) : success
 */
int IdeDaemonStartUp()
{
    IDE_EVENT("Attempt to fork child process");
    pid_t pid = IdeFork();
    if (pid < 0) {
        IDE_LOGE("fork child process failed");
        return IDE_DAEMON_ERROR;
    } else if (pid > 0) {
        return 0;
    }

    pid_t sid = setsid();
    if (sid < 0) {
        char errBuf[MAX_ERRSTR_LEN + 1] = {0};
        IDE_LOGE("setsid failed, errmsg: %s", mmGetErrorFormatMessage(mmGetErrorCode(), errBuf, MAX_ERRSTR_LEN));
        return IDE_DAEMON_ERROR;
    }
    if (Adx::Manager::Config::AdxConfigManager::Instance().Init() == false) {
        IDE_LOGE("config file prase failed");
        return IDE_DAEMON_ERROR;
    }
    // Reset file creation mask
    if (Adx::Manager::Config::AdxConfigManager::Instance().GetInstallMode() == 0) {
        mmUmask(IdeDaemon::Common::Config::DEFAULT_UMASK);
    } else {
        mmUmask(IdeDaemon::Common::Config::SPECIAL_UMASK);
    }
    std::string pidLockFile;
    int fd = SingleProcessStart(pidLockFile);
    if (fd < 0) {
        IDE_LOGE("single process start error");
        (void)IdeRealFileRemove(pidLockFile.c_str());
        return IDE_DAEMON_ERROR;
    }
    dlog_init(); // notify slogd that pid was changed
    LogAttr logAttr;
    logAttr.type = SYSTEM;
    logAttr.pid = 0;
    logAttr.deviceId = 0;
    if (DlogSetAttr(logAttr) != IDE_DAEMON_OK) {
        IDE_LOGW("Set log attr failed.");
    }
    int err = AdxStartUpInit();
    // destory daemon
    DaemonDestroy();
    IDE_MMCLOSE_AND_SET_INVALID(fd);
    (void)IdeRealFileRemove(pidLockFile.c_str());
    IDE_LOGI("ada start up exit");
    return err;
}

#if defined(__IDE_UT) || defined(__IDE_ST)
int IdeDaemonTestMain(int argc, IdeStringBuffer argv[])
#else
int main(int /* argc */, IdeStrBufAddrT /* argv[] */)
#endif
{
    return IdeDaemonStartUp();
}

