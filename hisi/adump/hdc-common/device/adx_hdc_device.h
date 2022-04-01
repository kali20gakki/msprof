/**
 * @file adx_hdc_device.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_HDC_DEVICE_H
#define ADX_HDC_DEVICE_H
#include <mutex>
#include <vector>
#include "adx_device.h"
namespace Adx {
class AdxHdcDevice : public AdxDevice {
public:
    AdxHdcDevice() {}
    ~AdxHdcDevice() override {}
    void GetAllEnableDevices(std::vector<std::string> &devices) override;
private:
    std::mutex deviceNotifyMtx_;
};
}
#endif