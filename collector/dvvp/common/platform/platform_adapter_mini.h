/**
* @file platform_adapter_mini.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_MINI_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_MINI_H

#include "platform_adapter_interface.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {

class PlatformAdapterMini : public Collector::Dvvp::Common::PlatformAdapter::PlatformAdapterInterface {
public:
    PlatformAdapterMini();
    ~PlatformAdapterMini();

    int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params) override;
    int Uninit();

private:
    std::string GenerateCapacityEvents();
    std::string GenerateBandwidthEvents();
};
}
}
}
}
#endif