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
#include "nlohmann/json.hpp"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/utils/file.h"
#include "device_context.h"
#include "device_context_error_code.h"

using namespace Analysis;
using namespace Analysis::Utils;
using namespace Analysis::Domain;


namespace nlohmann {

template <>
struct adl_serializer<DeviceInfo> {
    static void to_json(json& jsonData, const DeviceInfo& infoData)
    {
        jsonData = json{{"platform_version", infoData.chipID}, {"devices", infoData.deviceId}};
    }
    static void from_json(const json& jsonData, DeviceInfo& infoData)
    {
        int fromJsonResult = ANALYSIS_OK;
        std::string platformVersionStr = jsonData.at("platform_version").get<std::string>();
        fromJsonResult |= StrToU32(infoData.chipID, platformVersionStr);

        std::string deviceStr = jsonData.at("devices").get<std::string>();
        fromJsonResult |= StrToU32(infoData.deviceId, deviceStr);

        jsonData.at("DeviceInfo").at(0).at("ai_core_num").get_to(infoData.aiCoreNum);

        std::string aicFrequencyStr = jsonData.at("DeviceInfo").at(0).at("aic_frequency").get<std::string>();
        fromJsonResult |= StrToU32(infoData.aicFrequency, aicFrequencyStr);

        jsonData.at("DeviceInfo").at(0).at("aiv_num").get_to(infoData.aivNum);

        std::string aivFrequencyStr = jsonData.at("DeviceInfo").at(0).at("aiv_frequency").get<std::string>();
        fromJsonResult |= StrToU32(infoData.aivFrequency, aivFrequencyStr);

        std::string hwtsFrequencyStr = jsonData.at("DeviceInfo").at(0).at("hwts_frequency").get<std::string>();
        fromJsonResult |= StrToDouble(infoData.hwtsFrequency, hwtsFrequencyStr);
        if (fromJsonResult != ANALYSIS_OK) {
            ERROR("Get device info from json error! The input: platform_version= %, devices= %, aic_frequency= %, "
                  "aiv_frequency= %, hwts_frequency=%", platformVersionStr, deviceStr, aicFrequencyStr, aivFrequencyStr,
                  hwtsFrequencyStr);
            throw std::runtime_error("Get info json failed");
        }
    }
};

}

namespace Analysis {

namespace Domain {

const std::string INFO_JSON = "info.json";

bool DeviceContext::GetInfoJson()
{
    std::vector <std::string> files = Analysis::Utils::File::GetOriginData(deviceContextInfo.deviceFilePath,
                                                                           {INFO_JSON}, {"done"});
    if (files.size() != 1) {
        ERROR("The number of % in % is invalid.", INFO_JSON, deviceContextInfo.deviceFilePath);
        return false;
    }

    FileReader fd(files.back());
    nlohmann::json info;
    if (fd.ReadJson(info) != ANALYSIS_OK) {
        ERROR("Load json context failed: '%'.", files.back());
        return false;
    }

    try {
        deviceContextInfo.deviceInfo = info;
    } catch (const std::exception& e) {
        ERROR("Error parsing JSON: '%'.", e.what());
        return false;
    }
    return true;
}

}

}
