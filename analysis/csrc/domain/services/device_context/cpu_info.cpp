/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : cpu_info.cpp
 * Description        : 结构化context模块
 * Author             : msprof team
 * Creation Date      : 2024/5/28
 * *****************************************************************************
 */
#include "nlohmann/json.hpp"
#include "analysis/csrc/infrastructure/utils/utils.h"
#include "analysis/csrc/infrastructure/utils/file.h"
#include "device_context.h"
#include "device_context_error_code.h"

using namespace Analysis;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace nlohmann {

template <>
struct adl_serializer<CpuInfo> {
    static void to_json(json& jsonData, const CpuInfo& infoData)
    {
        jsonData = json{{"Frequency", infoData.frequency}};
    }
    static void from_json(const json& jsonData, CpuInfo& infoData)
    {
        std::string frequencyStr = jsonData.at("CPU").at(0).at("Frequency").get<std::string>();
        if (!frequencyStr.empty()) {
            if (StrToDouble(infoData.frequency, frequencyStr) != ANALYSIS_OK) {
                ERROR("Get frequency failed, the input string is '%'.", frequencyStr);
            }
        }
    }
};
}

namespace Analysis {
namespace Domain {

const std::string INFO_JSON = "info.json";

bool DeviceContext::GetCpuInfo()
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
        deviceContextInfo.cpuInfo = info;
    } catch (const std::exception& e) {
        ERROR("Error parsing JSON: '%'.", e.what());
        return false;
    }
    return true;
}

}

}