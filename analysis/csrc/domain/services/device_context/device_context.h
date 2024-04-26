/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_context.h
 * Description        : device侧上下文基类
 * Author             : msprof team
 * Creation Date      : 2024/4/26
 * *****************************************************************************
 */

#ifndef ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_DEVICE_CONTEXT_H
#define ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_DEVICE_CONTEXT_H

#include "analysis/csrc/infrastructure/context/include/context.h"

namespace Analysis {
namespace Domain {

struct DeviceInfo {
    uint32_t chipID;
};

struct DfxInfo {
    std::string stopAt;
};

struct DeviceContextInfo {
    std::string deviceFilePath;
    DeviceInfo deviceInfo;
    DfxInfo dfxInfo;
};

class DeviceContext : public Infra::Context {
public:
    DeviceContextInfo deviceContextInfo;
    uint32_t GetChipID() const override { return deviceContextInfo.deviceInfo.chipID; }
    std::string GetDeviceFilePath() const { return this->deviceContextInfo.deviceFilePath; }
    const std::string& GetDfxStopAtName() const override { return deviceContextInfo.dfxInfo.stopAt; }
};

}
}

#endif // ANALYSIS_DOMAIN_SERVICES_DEVICE_CONTEXT_DEVICE_CONTEXT_H
