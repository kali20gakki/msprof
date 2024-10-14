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
#include <iostream>
#include <sstream>
#include "nlohmann/json.hpp"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/utils/file.h"
#include "device_context.h"
#include "device_context_error_code.h"

using namespace Analysis;
using namespace Analysis::Utils;
using namespace Analysis::Domain;

namespace Analysis {

namespace Domain {
namespace {
const std::string SAMPLE_JSON = "sample.json";
const char EVENT_SEPARATOR = ',';
}

// 分割字符串，提取十六进制数
void HexStrToInt(std::string &jsonStr, std::vector<uint32_t>& intValues)
{
    std::stringstream ss(jsonStr);
    int index = 0;
    while (!ss.eof()) {
        char comma;
        uint32_t value;
        ss >> std::hex >> value;
        if (ss.fail()) {
            ERROR("The MetricEventsStr error, please check!");
            break;
        }
        if (ss.peek() == EVENT_SEPARATOR) {
            ss >> comma;
        }
        if (index < DEFAULT_PMU_LENGTH) {
            intValues[index] = value;
        } else {
            intValues.push_back(value);
        }
        ++index;
    }
}

AivMetricsEventsType GetAivMetricsEventsTypeFromStr(const std::string &aivMetrics)
{
    auto it = aivMetricsMap.find(aivMetrics);
    if (it != aivMetricsMap.end()) {
        return it->second;
    } else {
        // 如果找不到对应的枚举值，返回AicMetricsUnkown
        return AivMetricsEventsType::AIV_METRICS_UNKNOWN;
    }
}

AicMetricsEventsType GetAicMetricsEventsTypeFromStr(const std::string &aicMetrics)
{
    auto it = aicMetricsMap.find(aicMetrics);
    if (it != aicMetricsMap.end()) {
        return it->second;
    } else {
        // 如果找不到对应的枚举值，返回AicMetricsUnkown
        return AicMetricsEventsType::AIC_METRICS_UNKNOWN;
    }
}

ProfilingMode GetProfilingModeFromStr(const std::string &profilingStr)
{
    auto it = proflingMap.find(profilingStr);
    if (it != proflingMap.end()) {
        return it->second;
    } else {
        // 如果找不到对应的枚举值，PROFILING_UNKOWN
        return ProfilingMode::PROFILING_UNKOWN;
    }
}

}

}

namespace nlohmann {

template <>
struct adl_serializer<SampleInfo> {
    static void to_json(json& jsonData, const SampleInfo& infoData)
    {
        jsonData = json{{"ai_core_profiling", infoData.aiCoreProfiling}, {"ai_core_metrics", infoData.aiCoreMetrics}};
    }
    static void from_json(const json& jsonData, SampleInfo& infoData)
    {
        std::string aiCoreProfilingStr = jsonData.at("ai_core_profiling").get<std::string>();
        infoData.aiCoreProfiling = (aiCoreProfilingStr == "on");

        std::string aiCoreMetricsStr = jsonData.at("ai_core_metrics").get<std::string>();
        infoData.aiCoreMetrics = GetAicMetricsEventsTypeFromStr(aiCoreMetricsStr);

        jsonData.at("ai_core_profiling_events").get_to(infoData.aiCoreProfilingEventsStr);
        if (infoData.aiCoreProfilingEventsStr != "") {
            HexStrToInt(infoData.aiCoreProfilingEventsStr, infoData.aiCoreProfilingEvents);
        }

        std::string profilingStr = jsonData.at("ai_core_profiling_mode").get<std::string>();
        infoData.aiCoreProfilingMode = GetProfilingModeFromStr(profilingStr);

        jsonData.at("aicore_sampling_interval").get_to(infoData.aiCoreSamplingInterval);

        std::string aivProfilingStr = jsonData.at("aiv_profiling").get<std::string>();
        infoData.aivProfiling = (aivProfilingStr == "on");

        std::string aivMetricsStr = jsonData.at("aiv_metrics").get<std::string>();
        infoData.aivMetrics = GetAivMetricsEventsTypeFromStr(aivMetricsStr);

        jsonData.at("aiv_profiling_events").get_to(infoData.aivProfilingEventsStr);
        if (infoData.aivProfilingEventsStr != "") {
            HexStrToInt(infoData.aivProfilingEventsStr, infoData.aivProfilingEvents);
        }

        std::string aivProfilingMode = jsonData.at("aiv_profiling_mode").get<std::string>();
        infoData.aivProfilingMode = GetProfilingModeFromStr(aivProfilingMode);

        jsonData.at("aiv_sampling_interval").get_to(infoData.aivSamplingInterval);
    }
};

}

bool DeviceContext::GetSampleJson()
{
    std::vector <std::string> files = File::GetOriginData(deviceContextInfo.deviceFilePath,
                                                          {SAMPLE_JSON}, {"done"});
    if (files.size() != 1) {
        ERROR("The number of % in % is invalid.", SAMPLE_JSON, deviceContextInfo.deviceFilePath);
        return false;
    }

    FileReader fd(files.back());
    nlohmann::json info;
    if (fd.ReadJson(info) != ANALYSIS_OK) {
        ERROR("Load json context failed: '%'.", files.back());
        return false;
    }

    try {
        deviceContextInfo.sampleInfo = info;
    } catch (const std::exception& e) {
        ERROR("Error parsing JSON: '%'.", e.what());
        return false;
    }

    return true;
}