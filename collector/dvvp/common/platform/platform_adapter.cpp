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

namespace Collector {
namespace Dvvp {
namespace Common {
namespace PlatformAdapter {
using namespace analysis::dvvp::common::error;
using namespace Collector::Dvvp::Common::PlatformAdapter;
using namespace analysis::dvvp::common::config;

PlatformAdapter::PlatformAdapter()
    : platformType_(Analysis::Dvvp::Common::Config::PlatformType::END_TYPE), params_(nullptr)
{
}

PlatformAdapter::~PlatformAdapter()
{
}

SHARED_PTR_ALIA<PlatformAdapterInterface> PlatformAdapter::Init(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    Analysis::Dvvp::Common::Config::PlatformType platformType)
{
    return nullptr;
}

int PlatformAdapter::Uninit()
{
    return PROFILING_SUCCESS;
}

}
}
}
}