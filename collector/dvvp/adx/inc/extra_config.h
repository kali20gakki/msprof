/**
 * @file extra_config.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef IDE_DAEMON_COMMON_EXTRA_CONFIG_H
#define IDE_DAEMON_COMMON_EXTRA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void*                IdeSession;
typedef void*                IdeThreadArg;
typedef void*                IdeMemHandle;
typedef void*                IdeBuffT;
typedef void**               IdeRecvBuffT;
typedef const void*          IdeSendBuffT;
typedef int*                 IdeI32Pt;
typedef unsigned int*        IdeU32Pt;

using IdeStringBuffer = char *;
using IdeString = const char *;
using IdeU8Pt = unsigned char *;

const int IDE_DAEMON_ERROR = -1;
const int IDE_DAEMON_OK = 0;
const int IDE_DAEMON_SOCK_CLOSE = 1;
const int IDE_DAEMON_RECV_NODATA = 2;   // 2 : no data
const int MAX_SESSION_NUM = 96; // 96 : max session num

#ifdef __cplusplus
}
#endif

#endif

