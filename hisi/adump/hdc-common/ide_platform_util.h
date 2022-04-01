/**
 * @file ide_platform_util.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef IDE_PLATFORM_UTIL_H
#define IDE_PLATFORM_UTIL_H

#include <string>
#include "hdc_api.h"
#include "common/utils.h"
#include "common/extra_config.h"
#if (OS_TYPE != WIN)
#include <fcntl.h>
#endif
namespace Adx {
enum class IdeChannel {
    IDE_CHANNEL_SOCK,
    IDE_CHANNEL_HDC,
    IDE_CHANNEL_LOCAL,
};

struct IdeTransChannel {
    IdeChannel type;
    IdeSession session;
};

int IdeRead(const struct IdeTransChannel &handle, IdeRecvBuffT readBuf, IdeI32Pt readLen, int flag);
int IdeGetWorkspace(IdeStringBuffer path, uint32_t len);
int IdeRealFileRemove(IdeString file);
std::string IdeGetHomeDir();

#if (OS_TYPE != WIN)
pid_t IdeFork(void);
int IdeFcntl(int fd, int cmd, long arg);
int IdeLockFcntl(int fd, int cmd, const struct flock &lock);
#endif
}
#endif

