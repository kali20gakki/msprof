/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_context.h
 * Description        : 流程处理DEVICE侧上下文类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
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