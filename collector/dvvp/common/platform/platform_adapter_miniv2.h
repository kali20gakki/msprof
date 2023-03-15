/**
* @file platform_adapter_miniv2.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_MINIV2_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_MINIV2_H

#include "platform_adapter_interface.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
class PlatformAdapterMiniV2 : public Collector::Dvvp::Common::PlatformAdapter::PlatformAdapterInterface,
                              public analysis::dvvp::common::singleton::Singleton<PlatformAdapterMiniV2> {
public:
    PlatformAdapterMiniV2();
    ~PlatformAdapterMiniV2() override;

    int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params, PlatformType platformType) override;
    int Uninit() override;
};
}
}
}
}
#endif