/**
 * @file adx_comm_opt.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ADX_COMMOPT_H
#define ADX_COMMOPT_H
#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include "common/extra_config.h"
#include "device/adx_device.h"
#include "common_utils.h"
namespace Adx {
using OptHandle = uintptr_t;
constexpr int32_t COMM_OPT_BLOCK = 0;
constexpr int32_t COMM_OPT_NOBLOCK = 1;
constexpr OptHandle ADX_OPT_INVALID_HANDLE = -1;
const std::string OPT_DEVICE_KEY       = "DeviceId";
const std::string OPT_SERVICE_KEY      = "ServiceType";
const std::string OPT_PID_KEY          = "Pid";
enum class OptType {
    COMM_HDC,
    COMM_SSL,
    COMM_LOCAL,
    NR_COMM
};

class AdxCommOpt {
public:
    AdxCommOpt() : device_(nullptr) {}
    virtual ~AdxCommOpt() {}
    virtual std::string CommOptName() = 0;
    virtual OptType GetOptType() = 0;
    virtual OptHandle OpenServer(const std::map<std::string, std::string> &info) = 0;
    virtual int32_t CloseServer(const OptHandle &handle) const = 0;
    virtual OptHandle OpenClient(const std::map<std::string, std::string> &info) = 0;
    virtual int32_t CloseClient(OptHandle &handle) const = 0;
    virtual OptHandle Accept(const OptHandle handle) const = 0;
    virtual OptHandle Connect(const OptHandle handle, const std::map<std::string, std::string> &info) = 0;
    virtual int32_t Close(OptHandle &handle) const = 0;
    virtual int32_t Write(const OptHandle handle, IdeSendBuffT buffer, int32_t length, int32_t flag) = 0;
    virtual int32_t Read(const OptHandle handle, IdeRecvBuffT buffer, int32_t &length, int32_t flag) = 0;
    virtual SharedPtr<AdxDevice> GetDevice() = 0;
    virtual void Timer(void) const = 0;
protected:
    SharedPtr<AdxDevice> device_ = nullptr;
};
}
#endif
