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
#ifndef ANALYSIS_INFRASTRUCTURE_CONTEXT_INCLUDE_DEVICE_CONTEXT_H
#define ANALYSIS_INFRASTRUCTURE_CONTEXT_INCLUDE_DEVICE_CONTEXT_H

#include "analysis/csrc/infrastructure/context/include/context.h"

namespace Analysis {

namespace Infra {

struct DeviceInfo {
    uint32_t chipID;
};

struct DfxInfo {
    std::string stopAt;
};

struct DeviceContextInfo {
    DeviceInfo deviceInfo;
    DfxInfo dfxInfo;
};

class DeviceContext : public Context {
public:
    DeviceContextInfo deviceContextInfo;
    uint32_t GetChipID() const override { return deviceContextInfo.deviceInfo.chipID; }

    const std::string& GetDfxStopAtName() const override { return deviceContextInfo.dfxInfo.stopAt; }
};

}

}

#endif