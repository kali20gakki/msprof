/**
* @file platform_adapter_lhisi.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_LHISI_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_LHISI_H

#include "platform_adapter.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
class PlatformAdapterLhisi : public Collector::Dvvp::Common::PlatformAdapter::PlatformAdapter {
public:
    PlatformAdapterLhisi();
    ~PlatformAdapterLhisi();

    int Init();
    int Uninit();

private:
    std::vector<CollectorTypes> commonSwitch_;
    Analysis::Dvvp::Common::Config::PlatformType platformType_;
};

}
}
}
#endif

