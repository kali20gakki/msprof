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
#include "platform_adapter_mini.h"
#include "platform_adapter_mdc.h"
#include "platform_adapter_lhisi.h"
#include "platform_adapter_dc.h"
#include "platform_adapter_cloud.h"
#include "platform_adapter_cloudv2.h"
#include "platform_adapter_miniv2.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace Analysis::Dvvp::Common::Config;
class PlatformAdapter : public analysis::dvvp::common::singleton::Singleton<PlatformAdapter> {
public:
    PlatformAdapter();
    ~PlatformAdapter();
    
    int Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params, PlatformType platformType);
    int Uninit();
    PlatformAdapterInterface* GetAdapter() const;
private:
    PlatformAdapterInterface* platformAdapter_;
    bool inited_;
};
}
}
}
}
#endif

