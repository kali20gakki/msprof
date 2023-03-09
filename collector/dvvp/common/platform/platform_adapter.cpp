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
#include "utils.h"

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::config;
using namespace Analysis::Dvvp::Common::Config;

PlatformAdapter::PlatformAdapter() : platformAdapter_(nullptr)
{
}

PlatformAdapter::~PlatformAdapter()
{
}

int PlatformAdapter::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params, PlatformType platformType)
{
    const std::map<PlatformType, PlatformAdapterInterface*> ADAPTER_LIST = {
        {PlatformType::MINI_TYPE, PlatformAdapterMini::instance()},
        {PlatformType::CLOUD_TYPE, PlatformAdapterCloud::instance()},
        {PlatformType::MDC_TYPE, PlatformAdapterMdc::instance()},
        {PlatformType::LHISI_TYPE, PlatformAdapterLhisi::instance()},
        {PlatformType::DC_TYPE, PlatformAdapterDc::instance()},
        {PlatformType::CHIP_V4_1_0, PlatformAdapterCloudV2::instance()},
        {PlatformType::CHIP_V4_2_0, PlatformAdapterMiniV2::instance()},
    };
    auto iter = ADAPTER_LIST.find(platformType);
    if (iter == ADAPTER_LIST.end()) {
        return PROFILING_FAILED;
    }
    if (iter->second->Init(params, platformType) == PROFILING_SUCCESS) {
        platformAdapter_ = iter->second;
        return PROFILING_SUCCESS;
    }
    return PROFILING_FAILED;
}

int PlatformAdapter::Uninit()
{
    return PROFILING_SUCCESS;
}

PlatformAdapterInterface* PlatformAdapter::GetAdapter() const
{
    return platformAdapter_;
}

}
}
}
}