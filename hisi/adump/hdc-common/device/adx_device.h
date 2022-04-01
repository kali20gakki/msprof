/**
 * @file adx_device.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_DEVICE_H
#define ADX_DEVICE_H
#include <string>
#include <vector>
#include <map>
namespace Adx {
enum class DeviceState {
    NO_DEVICE_STATE,
    ENABLE_STATE,
    DISABLE_STATE,
};

class AdxDevice {
public:
    virtual ~AdxDevice() {}
    virtual void GetAllEnableDevices(std::vector<std::string> &devices) = 0;
    void EnableNotify(const std::string &devId);
    void DisableNotify(const std::string &devId);
    void GetEnableDevices(std::vector<std::string> &devices);
    void GetDisableDevices(std::vector<std::string> &devices);
    bool NoDevice();
    void InitDevice(const std::string &devId);
private:
    std::map<std::string, DeviceState> devices_;
};
}
#endif

