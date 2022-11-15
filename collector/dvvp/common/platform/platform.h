/**
* @file platform.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_COMMON_PLATFORM_H
#define ANALYSIS_DVVP_COMMON_PLATFORM_H

#include <pthread.h>
#include "singleton/singleton.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Platform {

inline void PthreadOnce(pthread_once_t *flag, void (*func)(void))
{
    (void)pthread_once(flag, func);
}

enum SysPlatformType {
    DEVICE = 0,
    HOST = 1,
    LHISI = 2,
    INVALID = 3
};

class Platform : public analysis::dvvp::common::singleton::Singleton<Platform> {
public:
    Platform();
    ~Platform();

    void Init();
    int Uninit();
    bool PlatformIsSocSide();
    bool PlatformIsRpcSide();
    bool PlatformIsHelperHostSide();
    bool RunSocSide();
    bool DriverAvailable();
    void SetPlatformSoc();
    uint32_t GetPlatform(void);
    int PlatformInitByDriver();

private:
    pthread_once_t callFlags_;
    bool isHelperHostSide_;
    bool driverAvailable_;
    uint32_t platformType_;
    uint32_t runSide_;
};
}
}
}
}
#endif

