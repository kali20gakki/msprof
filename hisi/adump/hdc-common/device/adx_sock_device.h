/**
 * @file adx_sock_device.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_SOCK_DEVICE_H
#define ADX_SOCK_DEVICE_H
#include <vector>
#include "adx_device.h"
namespace Adx {
class AdxSockDevice : public AdxDevice {
public:
    AdxSockDevice() {}
    ~AdxSockDevice() override {}
    void GetAllEnableDevices(std::vector<std::string> &devices) override;
};
}
#endif

