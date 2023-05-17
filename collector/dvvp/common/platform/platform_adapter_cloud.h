/**
* @file platform_adapter_cloud.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_CLOUD_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_CLOUD_H

#include "platform_adapter_interface.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
class PlatformAdapterCloud : public Collector::Dvvp::Common::PlatformAdapter::PlatformAdapterInterface,
                             public analysis::dvvp::common::singleton::Singleton<PlatformAdapterCloud> {
public:
    PlatformAdapterCloud();
    ~PlatformAdapterCloud() override;

    int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params, PlatformType platformType) override;
    int Uninit() override;
};
}
}
}
}
#endif

