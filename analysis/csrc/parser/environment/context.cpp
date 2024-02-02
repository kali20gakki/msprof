/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context.cpp
 * Description        : 采集的Context信息
 * Author             : msprof team
 * Creation Date      : 2023/11/20
 * *****************************************************************************
 */

#include "analysis/csrc/parser/environment/context.h"

#include <unordered_set>

#include "analysis/csrc/dfx/error_code.h"
#include "analysis/csrc/dfx/log.h"
#include "analysis/csrc/utils/utils.h"
#include "analysis/csrc/viewer/database/finals/unified_db_constant.h"
#include "collector/dvvp/common/config/config.h"

namespace Analysis {
namespace Parser {
namespace Environment {
using namespace Analysis;
using namespace Analysis::Utils;
using namespace Viewer::Database;

namespace {
const uint32_t ALL_EXPORT_VERSION = 0x072211;  // 2023年10月30号之后支持全导的驱动版本号
// 需要用到的json 和 log的文件名（前缀）
const std::string INFO_JSON = "info.json";
const std::string SAMPLE_JSON = "sample.json";
const std::string START_INFO = "start_info";
const std::string END_INFO = "end_info";
const std::string HOST_START_LOG = "host_start.log";
const std::string DEVICE_START_LOG = "dev_start.log";
// CHECK_VALUES存放一定存在的字段,字段不存在Load失败。
// 已经校验过的字段,可直接使用.at();未被校验过的字段则使用.value(),来设置默认值。确保读取正常
const std::set<std::string> CHECK_VALUES = {
    "platform_version", "startCollectionTimeBegin", "endCollectionTimeEnd",
    "startClockMonotonicRaw", "pid", "cntvct", "CPU", "DeviceInfo", "clock_monotonic_raw"
};
}

bool Context::Load(const std::set<std::string> &profPaths)
{
    for (const auto &profPath : profPaths) {
        std::vector<std::string> deviceDirs = File::GetOriginData(profPath, {HOST, DEVICE_PREFIX}, {});
        for (const auto &deviceDir: deviceDirs) {
            uint16_t deviceId = Utils::GetDeviceIdByDevicePath(deviceDir);
            if (!LoadJsonData(profPath, deviceDir, deviceId)) {
                return false;
            }
            if (!LoadLogData(profPath, deviceDir, deviceId)) {
                return false;
            }
            if (!CheckInfoValueIsValid(profPath, deviceId)) {
                return false;
            }
        }
    }
    return true;
}

bool Context::LoadJsonData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId)
{
    for (const auto &fileName: {INFO_JSON, SAMPLE_JSON, START_INFO, END_INFO}) {
        std::vector<std::string> files = File::GetOriginData(deviceDir, {fileName}, {"done"});
        if (files.size() != 1) {
            ERROR("The number of % in % is invalid.", fileName, deviceDir);
            return false;
        }
        FileReader fd(files.back());
        nlohmann::json content;
        if (fd.ReadJson(content) != ANALYSIS_OK) {
            ERROR("Load json context failed: '%'.", files.back());
            return false;
        }
        context_[profPath][deviceId].merge_patch(content);
        if (fileName == START_INFO) {
            context_[profPath][deviceId]["startClockMonotonicRaw"] =
                    context_[profPath][deviceId]["clockMonotonicRaw"];
            context_[profPath][deviceId]["startCollectionTimeBegin"] =
                    context_[profPath][deviceId]["collectionTimeBegin"];
        } else if (fileName == END_INFO) {
            context_[profPath][deviceId]["endClockMonotonicRaw"] =
                    context_[profPath][deviceId]["clockMonotonicRaw"];
            context_[profPath][deviceId]["endCollectionTimeEnd"] =
                    context_[profPath][deviceId]["collectionTimeEnd"];
        }
    }
    return true;
}

bool Context::LoadLogData(const std::string &profPath, const std::string &deviceDir, uint16_t deviceId)
{
    const int expectTokenSize = 2; // 2代表：前后的key和value
    // host就用host_start_log，device就用device_start_log。
    std::string fileNamePrefix = DEVICE_START_LOG;
    if (deviceId == HOST_ID) {
        fileNamePrefix = HOST_START_LOG;
    }
    std::vector<std::string> files = File::GetOriginData(deviceDir, {fileNamePrefix}, {"done"});
    // host、device底下只有1份log
    if (files.size() != 1) {
        ERROR("The number of % in % is invalid.", fileNamePrefix, deviceDir);
        return false;
    }
    FileReader fd(files.back());
    std::vector<std::string> text;
    if (fd.ReadText(text) != ANALYSIS_OK) {
        ERROR("Load log text failed: '%'.", files.back());
        return false;
    }
    for (const auto &line : text) {
        auto tokens = Utils::Split(line, ":");
        if (tokens.size() != expectTokenSize) {
            continue;
        }
        context_[profPath][deviceId][tokens[0]] = tokens[1];
    }
    return true;
}

bool Context::CheckInfoValueIsValid(const std::string &profPath, uint16_t deviceId)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    for (auto &valueName : CHECK_VALUES) {
        if (!info.contains(valueName)) {
            ERROR("The key called %, not in the context info, "
                  "the ProfPath is %, DeviceId is %.", valueName, profPath, deviceId);
            return false;
        }
        if (deviceId == HOST_ID) {
            if (!info.at("CPU").is_array() || (info.at("CPU").size() != 1)) {
                ERROR("CPU's value is invalid, "
                      "the ProfPath is %, DeviceId is %.", profPath, deviceId);
                return false;
            }
            if (!info.at("CPU").back().contains("Frequency")) {
                ERROR("There are no Frequency in context info, "
                      "the ProfPath is %, DeviceId is %.", valueName, profPath, deviceId);
                return false;
            }
        } else {
            if (!info.at("DeviceInfo").is_array() || (info.at("DeviceInfo").size() != 1)) {
                ERROR("DeviceInfo's value is invalid, "
                      "the ProfPath is %, DeviceId is %.", profPath, deviceId);
                return false;
            }
            auto freqArr = info.at("DeviceInfo").back();
            if (!freqArr.contains("hwts_frequency") || freqArr.at("hwts_frequency").empty()) {
                ERROR("There are no hwts_frequency in context info, "
                      "the ProfPath is %, DeviceId is %.", valueName, profPath, deviceId);
                return false;
            }
        }
    }
    return true;
}

bool Context::IsAllExport()
{
    const auto &info = GetInfoByDeviceId(HOST_ID);
    if (info.empty()) {
        return false;
    }
    // 低版本的驱动中不存在drvVersion字段，该字段使用时需要默认值
    auto drvVersion = info.value("drvVersion", 0u);
    if (drvVersion < ALL_EXPORT_VERSION) {
        return false;
    }
    uint16_t chip;
    if (StrToU16(chip, info.at("platform_version")) != ANALYSIS_OK) {
        ERROR("str to uint16_t failed.");
        return false;
    }
    if (chip == static_cast<uint16_t>(Chip::CHIP_V1_1_0) ||
            chip == static_cast<uint16_t>(Chip::CHIP_V3_1_0) ||
            chip == static_cast<uint16_t>(Chip::CHIP_V1_1_3)) {
        return false;
    }
    return true;
}

nlohmann::json Context::GetInfoByDeviceId(uint16_t deviceId, const std::string &profPath)
{
    nlohmann::json emptyJson;
    auto profInfo = profPath.empty() ? context_.begin() : context_.find(profPath);
    if (profInfo == context_.end()) {
        ERROR("Can't find PROF file. Please check your input path %.", profPath);
        return emptyJson;
    }
    auto deviceInfo = profInfo->second;
    if (deviceInfo.find(deviceId) == deviceInfo.end()) {
        ERROR("Can't find any info data in %, which device id is %.", profPath, deviceId);
        return emptyJson;
    }
    return deviceInfo[deviceId];
}

uint16_t Context::GetPlatformVersion(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    uint16_t platformVersion = UINT16_MAX;
    if (info.empty()) {
        return platformVersion;
    }
    if (StrToU16(platformVersion, info.at("platform_version")) != ANALYSIS_OK) {
        ERROR("PlatformVersion to uint16_t failed.");
    }
    return platformVersion;
}

bool Context::GetProfTimeRecordInfo(Utils::ProfTimeRecord &record, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(HOST_ID, profPath);
    if (info.empty()) {
        return false;
    }
    uint64_t startTimeUs = UINT64_MAX;
    if (StrToU64(startTimeUs, info.at("startCollectionTimeBegin")) != ANALYSIS_OK) {
        ERROR("StartTime to uint64_t failed.");
        return false;
    }
    uint64_t endTimeUs = 0;
    if (StrToU64(endTimeUs, info.at("endCollectionTimeEnd")) != ANALYSIS_OK) {
        ERROR("EndTime to uint64_t failed.");
        return false;
    }
    uint64_t baseTimeNs = UINT64_MAX;
    if (StrToU64(baseTimeNs, info.at("startClockMonotonicRaw")) != ANALYSIS_OK) {
        ERROR("BaseTime to uint64_t failed.");
        return false;
    }
    // 先判断时间之间的大小关系，确保后续计算时整数不回绕
    if ((startTimeUs * MILLI_SECOND < baseTimeNs) || (startTimeUs * MILLI_SECOND - baseTimeNs < TIME_BASE_OFFSET_NS)) {
        ERROR("The value of startTimeUs and baseTimeNs is invalid.");
        return false;
    }
    // startInfo endInfo 里的 collectionTime的单位是us，需要转换成ns
    record.startTimeNs = startTimeUs * MILLI_SECOND;
    record.endTimeNs = endTimeUs * MILLI_SECOND;
    record.baseTimeNs = baseTimeNs;
    return true;
}

uint64_t Context::GetPidFromInfoJson(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    uint64_t pid = 0;
    if (info.empty()) {
        return pid;
    }
    if (StrToU64(pid, info.at("pid")) != ANALYSIS_OK) {
        ERROR("Pid to uint64_t failed.");
    }
    return pid;
}

int64_t Context::GetMsBinPid(uint16_t deviceId, const std::string &profPath)
{
    const auto &info = GetInfoByDeviceId(deviceId, profPath);
    if (info.empty()) {
        ERROR("Samplejson is empty, please cheak your path %.", profPath);
        return analysis::dvvp::common::config::MSVP_MMPROCESS;
    }
 
    return info.value("msprofBinPid", analysis::dvvp::common::config::MSVP_MMPROCESS);
}

bool Context::GetSyscntConversionParams(Utils::SyscntConversionParams &params,
                                        uint16_t deviceId, const std::string &profPath)
{
    auto info = GetInfoByDeviceId(deviceId, profPath);
    // 根据deviceId取对应的文件中的数据
    if (info.empty()) {
        return false;
    }
    // cntvct 来自 start_log
    if (StrToU64(params.sysCnt, info.at("cntvct")) != ANALYSIS_OK) {
        ERROR("SysCnt to uint64_t failed.");
        return false;
    }
    if (deviceId == HOST_ID) {
        // freq 来自info.json 若为空则会被设置为 默认值 1000.0
        std::string freq = info.at("CPU").back().at("Frequency");
        if (freq.empty()) {
            INFO("HostFreq is empty, it will be set 1000.0 .");
        } else if (StrToDouble(params.freq, freq) != ANALYSIS_OK) {
            ERROR("HostFreq to double failed.");
            return false;
        }
    } else {
        // freq 来自info.json
        if (StrToDouble(params.freq, info.at("DeviceInfo").back().at("hwts_frequency")) != ANALYSIS_OK) {
            ERROR("DeviceFreq to double failed.");
            return false;
        }
        // 固定取host的clock_monotonic_raw
        info = GetInfoByDeviceId(HOST_ID, profPath);
        if (info.empty()) {
            ERROR("No host info.");
            return false;
        }
    }
    if (IsDoubleEqual(params.freq, 0)) {
        ERROR("Freq is 0, can't be used to calculate.");
        return false;
    }
    // clock_monotonic_raw 来自host目录下的 host_start_log
    if (StrToU64(params.hostMonotonic, info.at("clock_monotonic_raw")) != ANALYSIS_OK) {
        ERROR("HostMonotonic to uint64_t failed.");
        return false;
    }
    return true;
}

void Context::Clear()
{
    context_.clear();
}

bool Context::IsStarsChip(uint16_t platformVersion)
{
    return IsChipV1(platformVersion) || IsChipV4(platformVersion);
}

bool Context::IsChipV1(uint16_t platformVersion)
{
    auto chipV1_1_1 = static_cast<int>(Chip::CHIP_V1_1_0);
    auto chipV1_1_2 = static_cast<int>(Chip::CHIP_V1_1_2);
    auto chipV1_1_3 = static_cast<int>(Chip::CHIP_V1_1_3);
    std::unordered_set<int> checkList{chipV1_1_1, chipV1_1_2, chipV1_1_3};
    return static_cast<bool>(checkList.count(platformVersion));
}

bool Context::IsChipV4(uint16_t platformVersion)
{
    return platformVersion == static_cast<int>(Chip::CHIP_V4_1_0);
}

}  // namespace Environment
}  // namespace Parser
}  // namespace Analysis