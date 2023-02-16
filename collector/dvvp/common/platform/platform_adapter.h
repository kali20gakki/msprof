/**
* @file platform_adapter.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_H

#include "message/prof_params.h"
#include "config/config_manager.h"
#include "platform_adapter_interface.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
class PlatformAdapter {
public:
    PlatformAdapter();
    ~PlatformAdapter();
    SHARED_PTR_ALIA<PlatformAdapterInterface> Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
        Analysis::Dvvp::Common::Config::PlatformType platformType);
    int Uninit();
};
}
}
}
}
#endif

