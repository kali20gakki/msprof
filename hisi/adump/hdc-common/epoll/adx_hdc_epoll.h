/**
 * @file adx_hdc_epoll.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef ADX_HDC_EPOLL_H
#define ADX_HDC_EPOLL_H
#include "ascend_hal.h"
#include "adx_epoll.h"
namespace Adx {
#define HDC_EPOLL_EVENT_MASK (HDC_EPOLL_DATA_IN | HDC_EPOLL_CONN_IN | HDC_EPOLL_SESSION_CLOSE)
class AdxHdcEpoll : public AdxEpoll {
public:
    AdxHdcEpoll() : ep_(nullptr), epEvent_(nullptr) { epMaxSize_ = DEFAULT_EPOLL_SIZE; }
    ~AdxHdcEpoll() override {}
    int32_t EpollCreate(const int32_t size) override;
    int32_t EpollDestroy() override;
    int32_t EpollCtl(EpollHandle handle, EpollEvent &event, int32_t op) override;
    int32_t EpollAdd(EpollHandle target, EpollEvent &event) override;
    int32_t EpollDel(EpollHandle target, EpollEvent &event) override;
    int32_t EpollWait(EpollEvent events[], int32_t size, int32_t timeout) override;
    int32_t EpollErrorHandle() override;
    int32_t EpollGetSize() override;
private:
    uint32_t EpollEventToHdcEvent(uint32_t events) const;
    uint32_t HdcEventToEpollEvent(uint32_t events) const;
private:
    HDC_EPOLL ep_ = nullptr;
    struct drvHdcEvent *epEvent_ = nullptr;
};
}
#endif