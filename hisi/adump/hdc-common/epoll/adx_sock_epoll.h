/**
 * @file adx_sock_epoll.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_SOCK_EPOLL_H
#define ADX_SOCK_EPOLL_H
#include "adx_epoll.h"
namespace Adx {
#define SOCK_EPOLL_EVENT_MASK (HDC_EPOLL_DATA_IN | HDC_EPOLL_CONN_IN | HDC_EPOLL_SESSION_CLOSE)
class AdxSockEpoll : public AdxEpoll {
public:
    AdxSockEpoll() : ep_(-1)
    {
        epMaxSize_ = DEFAULT_EPOLL_SIZE;
    }
    ~AdxSockEpoll() override {}
    int32_t EpollCreate(const int32_t size) override;
    int32_t EpollAdd(EpollHandle target, EpollEvent &event) override;
    int32_t EpollDel(EpollHandle target, EpollEvent &event) override;
    int32_t EpollCtl(EpollHandle handle, EpollEvent &event, int32_t op) override;
    int32_t EpollWait(EpollEvent events[], int32_t size, int32_t timeout) override;
    int32_t EpollGetSize() override;
    int32_t EpollErrorHandle() override;
    int32_t EpollDestroy() override;
private:
    uint32_t HdcEventToEpollEvent(uint32_t events) const;
    uint32_t EpollEventToHdcEvent(uint32_t events) const;
private:
    int32_t ep_;
};
}
#endif