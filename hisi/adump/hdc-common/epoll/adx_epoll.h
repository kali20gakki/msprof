/**
 * @file adx_epoll.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_EPOLL_H
#define ADX_EPOLL_H
#include <cstdint>
#include <memory>
namespace Adx {
using EpollHandle = uintptr_t;
constexpr EpollHandle ADX_INVALID_HANDLE = -1;
constexpr int32_t DEFAULT_EPOLL_SIZE = 128;
constexpr int32_t DEFAULT_EPOLL_TIMEOUT = 500; // 500ms

constexpr uint32_t ADX_EPOLL_CONN_IN = 1U << 0;
constexpr uint32_t ADX_EPOLL_DATA_IN = 1U << 1;
constexpr uint32_t ADX_EPOLL_HANG_UP = 1U << 2;

struct EpollEvent {
    uint32_t events;
    EpollHandle data;
};

class AdxEpoll {
public:
    AdxEpoll() : epMaxSize_(DEFAULT_EPOLL_SIZE) {}
    virtual ~AdxEpoll() {}
    virtual int32_t EpollCreate(const int32_t size) = 0;
    virtual int32_t EpollDestroy() = 0;
    virtual int32_t EpollCtl(EpollHandle target, EpollEvent &event, int32_t op) = 0;
    virtual int32_t EpollAdd(EpollHandle target, EpollEvent &event) = 0;
    virtual int32_t EpollDel(EpollHandle target, EpollEvent &event) = 0;
    virtual int32_t EpollWait(EpollEvent events[], int32_t size, int32_t timeout) = 0;
    virtual int32_t EpollErrorHandle() = 0;
    virtual int32_t EpollGetSize() = 0;
protected:
    int epMaxSize_;
};
}
#endif