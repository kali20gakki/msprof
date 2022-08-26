/**
* @file platform_adapter.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#include "platform_adapter.h"
#include "prof_acl_api.h"
#include "errno/error_code.h"
#include "config/config.h"
#include "msprof_dlog.h"
#include "msprof_error_manager.h"
#include "platform_adapter_mini.h"
#include "platform_adapter_mdc.h"
#include "platform_adapter_lhisi.h"
#include "platform_adapter_dc.h"
#include "platform_adapter_cloud.h"
#include "platform_adapter_cloudv2.h"
#include "utils.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;

PlatformAdapter::PlatformAdapter()
{
}

PlatformAdapter::~PlatformAdapter()
{
}

SHARED_PTR_ALIA<PlatformAdapterInterface> PlatformAdapter::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    PlatformType platformType)
{
    SHARED_PTR_ALIA<PlatformAdapterInterface> platformAdapter = nullptr;
    if (platformType == PlatformType::MINI_TYPE) {
        MSVP_MAKE_SHARED0_RET(platformAdapter, PlatformAdapterMini, nullptr);
    } else if (platformType == PlatformType::CLOUD_TYPE) {
        MSVP_MAKE_SHARED0_RET(platformAdapter, PlatformAdapterCloud, nullptr);
    } else if (platformType == PlatformType::MDC_TYPE) {
        MSVP_MAKE_SHARED0_RET(platformAdapter, PlatformAdapterMdc, nullptr);
    } else if (platformType == PlatformType::LHISI_TYPE) {
        MSVP_MAKE_SHARED0_RET(platformAdapter, PlatformAdapterLhisi, nullptr);
    } else if (platformType == PlatformType::DC_TYPE) {
        MSVP_MAKE_SHARED0_RET(platformAdapter, PlatformAdapterDc, nullptr);
    } else if (platformType == PlatformType::CHIP_V4_1_0) {
        MSVP_MAKE_SHARED0_RET(platformAdapter, PlatformAdapterCloudV2, nullptr);
    }
    if (platformAdapter != nullptr) {
        int ret = platformAdapter->Init(params);
        if (ret != PROFILING_SUCCESS) {
            platformAdapter = nullptr;
        }
    }
    return platformAdapter;
}

int PlatformAdapter::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}