/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2025-2025
            Copyright, 2025, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : start_info.cpp
 * Description        : 读取start_info.x文件
 * Author             : msprof team
 * Creation Date      : 2025/4/3
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


const std::string DEVICE_START_INFO = "start_info";

namespace nlohmann {

template <>
struct adl_serializer<DeviceStartInfo> {
    static void to_json(json& jsonData, const DeviceStartInfo& infoData)
    {
        jsonData = json{{"clockMonotonicRaw", infoData.clockMonotonicRaw}};
    }
    static void from_json(const json& jsonData, DeviceStartInfo& infoData)
    {
        int fromJsonResult = 0;
        std::string collectionTimeBeginStr = jsonData.at("collectionTimeBegin").get<std::string>();
        fromJsonResult |= StrToU64(infoData.collectionTimeBegin, collectionTimeBeginStr);
        std::string clockMonotonicRawStr = jsonData.at("clockMonotonicRaw").get<std::string>();
        fromJsonResult |= StrToU64(infoData.clockMonotonicRaw, clockMonotonicRawStr);
        if (fromJsonResult != ANALYSIS_OK) {
            ERROR("Get device info from json error! The input: collectionTimeBegin= %, clockMonotonicRaw= %",
                  collectionTimeBeginStr, clockMonotonicRawStr);
            throw std::runtime_error("Get info json failed");
        }
    }
};
}

bool DeviceContext::GetStartInfo()
{
    std::vector<std::string> files = Analysis::Utils::File::GetOriginData(deviceContextInfo.deviceFilePath,
                                                                          {DEVICE_START_INFO}, {"done"});
    if (files.size() != 1) {
        ERROR("The number of % in % is invalid.", DEVICE_START_INFO, deviceContextInfo.deviceFilePath);
        return false;
    }
    FileReader fd(files.back());
    nlohmann::json info;
    if (fd.ReadJson(info) != ANALYSIS_OK) {
        ERROR("Load json context failed: '%'.", files.back());
        return false;
    }
    try {
        deviceContextInfo.startInfo = info;
    } catch (const std::exception &e) {
        ERROR("Error parsing JSON: '%'.", e.what());
        return false;
    }
    return true;
}
