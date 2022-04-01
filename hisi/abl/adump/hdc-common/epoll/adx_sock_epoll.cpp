/**
 * @file adx_sock_epoll.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_sock_epoll.h"
#include <sys/epoll.h>
#include "hdc_log.h"
namespace Adx {
int32_t AdxSockEpoll::EpollCreate(const int32_t size)
{
    if (size <= 0) {
        IDE_LOGE("epoll create size(%d) input error", size);
        return IDE_DAEMON_ERROR;
    }

    epMaxSize_ = size;
    int32_t fd = epoll_create(DEFAULT_EPOLL_SIZE);
    if (fd < 0) {
        return IDE_DAEMON_ERROR;
    }

    ep_ = fd;
    return IDE_DAEMON_OK;
}

int32_t AdxSockEpoll::EpollDestroy()
{
    return IDE_DAEMON_OK;
}
int32_t AdxSockEpoll::EpollCtl(EpollHandle handle, EpollEvent &event, int32_t op)
{
    if (ep_ == -1 || handle == ADX_INVALID_HANDLE) {
        IDE_LOGE("sock epoll ctl input error");
        return IDE_DAEMON_ERROR;
    }

    auto fd = static_cast<int32_t>(handle);
    if (fd < 0) {
        IDE_LOGE("sock epoll ctl handle error");
        return IDE_DAEMON_ERROR;
    }
    struct epoll_event ev = {0};
    ev.events = EpollEventToHdcEvent(event.events);
    ev.data.fd = fd;
    int32_t ret = epoll_ctl(ep_, op, fd, &ev);
    if (ret < 0) {
        IDE_LOGE("sock epoll ctl process error %d", ret);
    }
    return IDE_DAEMON_OK;
}

int32_t AdxSockEpoll::EpollAdd(EpollHandle target, EpollEvent &event)
{
    return EpollCtl(target, event, EPOLL_CTL_ADD);
}

int32_t AdxSockEpoll::EpollDel(EpollHandle target, EpollEvent &event)
{
    return EpollCtl(target, event, EPOLL_CTL_DEL);
}

int32_t AdxSockEpoll::EpollWait(EpollEvent events[], int32_t size, int32_t timeout)
{
    if (ep_ < 0) {
        IDE_LOGW("epoll wait init");
        return IDE_DAEMON_ERROR;
    }

    struct epoll_event epEvent[DEFAULT_EPOLL_SIZE] = { {0} };
    int32_t ret = epoll_wait(ep_, epEvent, size, timeout);
    if (ret < 0) {
        IDE_LOGE("epoll wait process error");
        return IDE_DAEMON_ERROR;
    }

    for (int32_t i = 0; i < ret && i < epMaxSize_; i++) {
        events[i].events = HdcEventToEpollEvent(epEvent[i].events);
        events[i].data = epEvent[i].data.fd;
    }

    return ret;
}

int32_t AdxSockEpoll::EpollErrorHandle()
{
    return IDE_DAEMON_OK;
}

int32_t AdxSockEpoll::EpollGetSize()
{
    return epMaxSize_;
}

uint32_t AdxSockEpoll::EpollEventToHdcEvent(uint32_t events) const
{
    uint32_t transed = ((events & ADX_EPOLL_DATA_IN) != 0) ? EPOLLIN : 0;
    transed |= ((events & ADX_EPOLL_CONN_IN) != 0) ? EPOLLIN : 0;
    transed |= ((events & ADX_EPOLL_HANG_UP) != 0) ? EPOLLHUP : 0;
    return transed;
}

uint32_t AdxSockEpoll::HdcEventToEpollEvent(uint32_t events) const
{
    uint32_t transed = ((events & EPOLLIN) != 0) ? ADX_EPOLL_DATA_IN : 0;
    transed |= ((events & EPOLLIN) != 0) ? ADX_EPOLL_CONN_IN : 0;
    transed |= ((events & EPOLLHUP) != 0) ? ADX_EPOLL_HANG_UP : 0;

    return transed;
}
}