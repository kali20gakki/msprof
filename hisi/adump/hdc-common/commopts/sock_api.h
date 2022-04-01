/**
 * @file sock_api.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef SOCK_API_H
#define SOCK_API_H
#include <cstdint>
#include <string>
#include "common/extra_config.h"
#define ADX_LOCAL_CLOSE_AND_SET_INVALID(fd) do {    \
    if ((fd) >= 0) {                                  \
        (void)close(fd);                            \
        fd = -1;                                    \
    }                                               \
} while (0)

int32_t SockServerCreate(const std::string &adxLocalChan);
int32_t SockServerDestroy(int32_t &sockFd);
int32_t SockClientCreate();
int32_t SockClientDestory(int32_t &sockFd);
int32_t SockAccept(int32_t sockFd);
int32_t SockConnect(int32_t sockFd, const std::string &adxLocalChan);
int32_t SockRead(int32_t fd, IdeRecvBuffT readBuf, IdeI32Pt recvLen, int32_t flag);
int32_t SockWrite(int32_t fd, IdeSendBuffT writeBuf, int32_t len, int32_t flag);
int32_t SockClose(int32_t &sockFd);
#endif