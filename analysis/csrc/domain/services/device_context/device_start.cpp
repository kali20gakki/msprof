/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : device_context.cpp
 * Description        : 结构化context模块
 * Author             : msprof team
 * Creation Date      : 2024/4/29
 * *****************************************************************************
 */

#include <unordered_map>
#include "nlohmann/json.hpp"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/utils/file.h"
#include "analysis/csrc/domain/services/device_context/device_context.h"
#include "analysis/csrc/domain/services/device_context/device_context_error_code.h"

namespace Analysis {
namespace Domain {
using namespace Analysis::Utils;

const std::string DEVICE_START_LOG = "dev_start.log";

bool serializeDeviceStart(DeviceStart &deviceStart, std::unordered_map<std::string, std::string> &deviceStartConfig)
{
    if (StrToU64(deviceStart.clockMonotonicRaw, deviceStartConfig["clock_monotonic_raw"]) != ANALYSIS_OK) {
        ERROR("DeviceMonotonic to uint64_t failed, invalid str is %.", deviceStartConfig["clock_monotonic_raw"]);
        return false;
    }

    if (StrToU64(deviceStart.sysCnt, deviceStartConfig["cntvct"]) != ANALYSIS_OK) {
        ERROR("SysCnt to uint64_t failed, invalid str is %.", deviceStartConfig["cntvct"]);
        return false;
    }
    return true;
}

bool DeviceContext::GetDeviceStart()
{
    std::vector <std::string> files = Analysis::Utils::File::GetOriginData(deviceContextInfo.deviceFilePath,
                                                                           {DEVICE_START_LOG}, {"done"});
    const int expectTokenSize = 2;
    if (files.size() != 1) {
        ERROR("The number of % in % is invalid.", DEVICE_START_LOG, deviceContextInfo.deviceFilePath);
        return false;
    }

    FileReader fd(files.back());
    std::vector<std::string> text;
    if (fd.ReadText(text) != ANALYSIS_OK) {
        ERROR("Load log text failed: '%'.", files.back());
        return false;
    }
    std::unordered_map<std::string, std::string> deviceStartConfig;
    for (const auto &line : text) {
        auto tokens = Utils::Split(line, ":");
        if (tokens.size() != expectTokenSize) {
            continue;
        }
        deviceStartConfig[tokens[0]] = tokens[1];
    }
    return serializeDeviceStart(deviceContextInfo.deviceStart, deviceStartConfig);
}
}
}