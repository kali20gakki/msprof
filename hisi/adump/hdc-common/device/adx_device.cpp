/**
 * @file adx_device.cpp
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "adx_device.h"
#include "log/adx_log.h"
namespace Adx {
void AdxDevice::EnableNotify(const std::string &devId)
{
    auto it = devices_.find(devId);
    if (it != devices_.end() && it->second != DeviceState::ENABLE_STATE) {
        IDE_LOGI("device %s going up.", devId.c_str());
        devices_[devId] = DeviceState::ENABLE_STATE;
    }
}

void AdxDevice::DisableNotify(const std::string &devId)
{
    auto it = devices_.find(devId);
    if (it != devices_.end() && it->second != DeviceState::DISABLE_STATE) {
        IDE_LOGI("device %s going suspend.", devId.c_str());
        devices_[devId] = DeviceState::DISABLE_STATE;
    }
}

void AdxDevice::GetEnableDevices(std::vector<std::string> &devices)
{
    auto it = devices_.begin();
    for (; it != devices_.end(); it++) {
        if (it->second == DeviceState::ENABLE_STATE) {
            devices.push_back(it->first);
        }
    }
}

void AdxDevice::GetDisableDevices(std::vector<std::string> &devices)
{
    devices.clear();
    auto it = devices_.begin();
    for (; it != devices_.end(); it++) {
        if (it->second == DeviceState::DISABLE_STATE) {
            devices.push_back(it->first);
        }
    }
}

bool AdxDevice::NoDevice()
{
    return devices_.empty();
}

void AdxDevice::InitDevice(const std::string &devId)
{
    auto it = devices_.find(devId);
    if (it == devices_.end()) {
        devices_[devId] = DeviceState::ENABLE_STATE;
    }
}
}
