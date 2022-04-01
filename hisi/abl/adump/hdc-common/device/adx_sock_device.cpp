/**
 * @file adx_sock_device.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_sock_device.h"
#include "log/adx_log.h"
namespace Adx {
void AdxSockDevice::GetAllEnableDevices(std::vector<std::string> &devices)
{
    devices.clear();
    if (AdxSockDevice::NoDevice()) {
        AdxSockDevice::InitDevice("0");
    }
    AdxSockDevice::GetEnableDevices(devices);
}
}
