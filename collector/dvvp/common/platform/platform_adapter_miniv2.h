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