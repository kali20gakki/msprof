/**
 * @file ide_platform_util.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "ide_platform_util.h"
#if (OS_TYPE != WIN)
#include <pwd.h>
#endif
#include "ide_common_util.h"
#include "common/config.h"

using namespace IdeDaemon::Common::Config;
namespace Adx {
/**
 * @brief ide read data, support socket and hdc
 * @param handle: socket handle or hdc session
 * @param read_buf: received buffer
 * @param read_len: the length of received buffer
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int IdeRead(const struct IdeTransChannel &handle, IdeRecvBuffT readBuf, IdeI32Pt readLen, int flag)
{
    int err;
    if (readBuf == nullptr || readLen == nullptr) {
        return IDE_DAEMON_ERROR;
    }

    if (flag == IDE_DAEMON_BLOCK) {
        err = HdcRead(handle.session, readBuf, readLen);
    } else {
        const int retryTimeMax = 142; // 10s (1+2+...+142)ms
        int retrySleepTime = 1;
        while (1) {
            err = HdcReadNb(handle.session, readBuf, readLen);
            if (err == IDE_DAEMON_RECV_NODATA) {
                mmSleep(retrySleepTime);
                retrySleepTime++;
            } else {
                return err;
            }
            if (retrySleepTime >= retryTimeMax) {
                IDE_LOGW("no received request in %d times", retrySleepTime);
                err = IDE_DAEMON_ERROR;
                break;
            }
        }
    }
    return err;
}

/**
 * @brief process command
 * @param cmd_info: command info
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
#if (OS_TYPE != WIN)
pid_t IdeFork(void)
{
    return fork();
}
#endif

/**
 * @brief provides control for (file) descriptors.
 * @param fd: file descriptors
 * @param cmd: F_GETFD/F_SETFD, see fcntl for details
 * @param arg: arguments for file descriptors
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
#if (OS_TYPE != WIN)
int IdeFcntl(int fd, int cmd, long arg)
{
    return fcntl(fd, cmd, arg);
}

/**
 * @brief provides control for (file) descriptors.
 * @param fd: file descriptors
 * @param cmd: F_GETLK/F_SETLK, see fcntl for details
 * @param lock: arguments for file Lock descriptors
 *
 * @return
 *        0: succ
 *        low 0: failed
 */
int IdeLockFcntl(int fd, int cmd, const struct flock &lock)
{
    return fcntl(fd, cmd, &lock);
}

#endif

/**
 * @brief get homedir
 *
 * @return
 *        string: home directory
 */
std::string IdeGetHomeDir()
{
#if (OS_TYPE != WIN)
    struct passwd *pw = getpwuid(getuid());
    if (pw != nullptr && pw->pw_dir != nullptr && strlen(pw->pw_dir) > 0 &&
        strlen(pw->pw_dir) < MMPA_MAX_PATH) {
        return pw->pw_dir;
    }

    return "";
#else
    char value[MAX_TMP_PATH] = { 0 };
    std::string defPath = IdeDevGetUserConfig(IDE_TEMP_KEY, WORKSPACE, value, sizeof(value));
    return defPath;
#endif
}

/**
 * @brief remove the file
 * @param file:file path
 *
 * @return
 *        IDE_DAEMON_OK: succ
 *        IDE_DAEMON_ERROR: failed
 */
int IdeRealFileRemove(IdeString file)
{
    if (file == nullptr || strlen(file) == 0) {
        return IDE_DAEMON_ERROR;
    }

    IdeStringBuffer actualPath = reinterpret_cast<IdeStringBuffer>(IdeXmalloc(MMPA_MAX_PATH));
    if (actualPath == nullptr) {
        IDE_LOGE("IdeXmalloc failed");
        return IDE_DAEMON_ERROR;
    }
    std::string path = IdeDaemon::Common::Utils::ReplaceWaveToHomeDir(file);
    int ret = mmRealPath(path.c_str(), actualPath, MMPA_MAX_PATH);
    if (ret != EN_OK) {
        IDE_XFREE_AND_SET_NULL(actualPath);
        return IDE_DAEMON_OK;
    }
    ret = remove(actualPath);
    IDE_CHECK_REMOVE(ret, actualPath);
    IDE_XFREE_AND_SET_NULL(actualPath);
    return IDE_DAEMON_OK;
}
}
