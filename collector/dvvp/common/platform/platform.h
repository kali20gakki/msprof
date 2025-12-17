/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/
#ifndef ANALYSIS_DVVP_COMMON_PLATFORM_H
#define ANALYSIS_DVVP_COMMON_PLATFORM_H

#include <pthread.h>
#include "singleton/singleton.h"

namespace Analysis {
namespace Dvvp {
namespace Common {
namespace Platform {
const std::string PROF_VERSION_INFO = "1.0";
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
    std::string PlatformGetHostOscFreq() const;
    std::string PlatformGetDeviceOscFreq(uint32_t deviceId, const std::string &freq) const;
    bool PlatformHostFreqIsEnable() const;

private:
    pthread_once_t callFlags_;
    bool isHelperHostSide_;
    bool driverAvailable_;
    uint32_t platformType_;
    uint32_t runSide_;
    bool enableHostOscFreq_;
    std::string hostOscFreq_;
};
}
}
}
}
#endif

