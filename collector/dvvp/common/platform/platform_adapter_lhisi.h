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

#include "platform_adapter_interface.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
class PlatformAdapterLhisi : public Collector::Dvvp::Common::PlatformAdapter::PlatformAdapterInterface {
public:
    PlatformAdapterLhisi();
    ~PlatformAdapterLhisi();

    int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        Analysis::Dvvp::Common::Config::PlatformType platformType) override;
    int Uninit();
};
}
}
}
}
#endif

