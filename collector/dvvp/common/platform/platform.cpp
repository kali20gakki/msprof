/**
* @file platform.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "errno/error_code.h"
#include "ai_drv_dev_api.h"
#include "platform/platform.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Platform {
using namespace analysis::dvvp::common::error;

Platform::Platform()
    : platformType_(SysPlatformType::INVALID),
      runSide_(SysPlatformType::INVALID),
      callFlags_(0),
      isHelperHostSide_(false),
      driverAvailable_(false),
      enableHostOscFreq_(false),
      hostOscFreq_(analysis::dvvp::driver::NOT_SUPPORT_FREQUENCY)
{
}

Platform::~Platform()
{
}

void Platform::Init()
{
    uint32_t platformType;
    MSPROF_EVENT("Profiling platform version: %s.", PROF_VERSION_INFO);
    if (analysis::dvvp::driver::DrvGetApiVersion() >= analysis::dvvp::driver::SUPPORT_OSC_FREQ_API_VERSION) {
        enableHostOscFreq_ = analysis::dvvp::driver::DrvGetHostFreq(hostOscFreq_);
    }
    driverAvailable_ = (analysis::dvvp::driver::DrvGetPlatformInfo(platformType) == PROFILING_SUCCESS) ? true : false;
    if (driverAvailable_) {
        runSide_ = platformType;
    }
    isHelperHostSide_ = analysis::dvvp::driver::DrvCheckIfHelperHost();
}

int Platform::Uninit()
{
    return PROFILING_SUCCESS;
}

bool Platform::PlatformIsSocSide()
{
    if (platformType_ == SysPlatformType::DEVICE || platformType_ == SysPlatformType::LHISI) {
        return true;
    }
    return false;
}

bool Platform::PlatformIsRpcSide()
{
    if (platformType_ == SysPlatformType::HOST) {
        return true;
    }
    return false;
}

bool Platform::PlatformIsHelperHostSide()
{
    PthreadOnce(&callFlags_, []() {Platform::instance()->Init();});
    return isHelperHostSide_;
}

bool Platform::RunSocSide()
{
    if (runSide_ == SysPlatformType::DEVICE || runSide_ == SysPlatformType::LHISI) {
        return true;
    }
    return false;
}

bool Platform::DriverAvailable()
{
    PthreadOnce(&callFlags_, []() {Platform::instance()->Init();});
    return driverAvailable_;
}

void Platform::SetPlatformSoc()
{
    (void)PlatformInitByDriver();
    platformType_ = SysPlatformType::DEVICE;
}

uint32_t Platform::GetPlatform(void)
{
    return platformType_;
}

int Platform::PlatformInitByDriver()
{
    int ret = analysis::dvvp::driver::DrvGetPlatformInfo(platformType_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("get platform info failed.");
        return PROFILING_FAILED;
    }
    runSide_ = platformType_;
    return PROFILING_SUCCESS;
}

std::string Platform::PlatformGetHostOscFreq() const
{
    return hostOscFreq_;
}

bool Platform::PlatformHostFreqIsEnable() const
{
    return enableHostOscFreq_;
}

std::string Platform::PlatformGetDeviceOscFreq(uint32_t deviceId, const std::string &freq) const
{
    std::string deviceOscFreq;
    bool enableDeviceOscFreq = false;
    if (enableHostOscFreq_) {
        enableDeviceOscFreq = analysis::dvvp::driver::DrvGetDeviceFreq(deviceId, deviceOscFreq);
    }

    return enableDeviceOscFreq ? deviceOscFreq : freq;
}
}
}
}
}