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
    : platformType_(SysPlatformType::INVALID), runSide_(SysPlatformType::INVALID)
{
}

Platform::~Platform()
{
}

int Platform::Init()
{
    return PROFILING_SUCCESS;
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

bool Platform::PlatformIsHelperHostSide() const
{
    return analysis::dvvp::driver::DrvCheckIfHelperHost();
}

bool Platform::RunSocSide()
{
    if (runSide_ == SysPlatformType::DEVICE || runSide_ == SysPlatformType::LHISI) {
        return true;
    }
    return false;
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
}
}
}
}