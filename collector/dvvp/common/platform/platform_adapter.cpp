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
    std::lock_guard<std::mutex> lk(mtx_);
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
    if (platformAdapter_ != nullptr) {
        return PROFILING_SUCCESS;
    }
    if (iter->second->Init(params, platformType) == PROFILING_SUCCESS) {
        platformAdapter_ = iter->second;
        return PROFILING_SUCCESS;
    }
    return PROFILING_FAILED;
}

int PlatformAdapter::Uninit()
{
    platformAdapter_ = nullptr;
    return PROFILING_SUCCESS;
}

PlatformAdapterInterface* PlatformAdapter::GetAdapter()
{
    std::lock_guard<std::mutex> lk(mtx_);
    return platformAdapter_;
}

}
}
}
}