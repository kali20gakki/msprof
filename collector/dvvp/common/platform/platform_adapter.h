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
#ifndef COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_H
#define COLLECTOR_DVVP_COMMON_PLATFORM_ADAPTER_H

#include <mutex>
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
    PlatformAdapterInterface* GetAdapter();
private:
    std::mutex mtx_;
    PlatformAdapterInterface* platformAdapter_;
};
}
}
}
}
#endif

