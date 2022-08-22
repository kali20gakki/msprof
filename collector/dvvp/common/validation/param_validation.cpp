/**
* @file uploader_mgr.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "param_validation.h"

#include "config/config.h"
#include "errno/error_code.h"
#include "message/codec.h"
#include "platform/platform.h"
#include "msprof_error_manager.h"
#include "config/config_manager.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace validation {
using namespace Analysis::Dvvp::Common::Platform;
using namespace Analysis::Dvvp::Common::Config;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

const int MIN_INTERVAL = 1;
const int MAX_INTERVAL = 15 * 24 * 3600 * 1000; // 15 * 24 * 3600 * 1000 = 15day's micro seconds
const int MAX_PERIOD = 30 * 24 * 3600; // 30 * 24 * 3600 = 30day's seconds
const int MAX_EVENT_SIZE = 8;  // every batch event size
const int MAX_CORE_ID_SIZE = 32;  // ai core or aiv core id size
const int BASE_HEX = 16;  // hex to int


ParamValidation::ParamValidation()
{
}

ParamValidation::~ParamValidation()
{
}

int ParamValidation::Init() const
{
    return PROFILING_SUCCESS;
}

int ParamValidation::Uninit() const
{
    return PROFILING_SUCCESS;
}

bool ParamValidation::CheckOutputIsValid(const std::string &outputPath)
{
    if (outputPath.empty()) {
        MSPROF_LOGI("output is empty");
        return true;
    }
    std::string path = Utils::CanonicalizePath(outputPath);
    if (!path.empty()) {
        if (path.size() > MAX_PATH_LENGTH) {
            MSPROF_LOGE("Invalid value for Argument 'output'. The maximum length is %d.", MAX_PATH_LENGTH);
            return false;
        }
        if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
            char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
            MSPROF_LOGE("Create output dir failed.ErrorCode: %d, ErrorInfo: %s.",
                MmGetErrorCode(),
                MmGetErrorFormatMessage(MmGetErrorCode(), errBuf, MAX_ERR_STRING_LEN));
            return false;
        }
        if (!Utils::IsDir(path)) {
            MSPROF_LOGE("Argument 'output' %s is not a dir.", path.c_str());
            return false;
        }
        if (MmAccess2(path, M_W_OK) != PROFILING_SUCCESS) {
            MSPROF_LOGE("Argument 'output' %s permission denied.", path.c_str());
            return false;
        }
    } else {
        MSPROF_LOGE("Argument 'output' is invalid. Failed to get the canonicalized absolute pathname.");
        return false;
    }
    return true;
}

bool ParamValidation::CheckStorageLimitIsValid(const std::string &storageLimit)
{
    if (storageLimit.empty()) {
        MSPROF_LOGI("storage_limit is empty");
        return true;
    }
    std::string errReason = "storage-limit should be in range [" + std::to_string(STORAGE_LIMIT_DOWN_THD) +
        "," + std::to_string(UINT32_MAX) + "], end with MB";

    uint32_t unitLen = strlen(STORAGE_LIMIT_UNIT);
    if (storageLimit.size() <= unitLen) {
        MSPROF_LOGE("storage_limit:%s, length is less than %u", storageLimit.c_str(), unitLen + 1);
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }

    std::string unitStr = storageLimit.substr(storageLimit.size() - unitLen);
    if (unitStr != STORAGE_LIMIT_UNIT) {
        MSPROF_LOGE("storage_limit:%s, not end with MB", storageLimit.c_str());
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }

    std::string digitStr = storageLimit.substr(0, storageLimit.size() - unitLen);
    if (!Utils::IsAllDigit(digitStr)) {
        MSPROF_LOGE("storage_limit:%s, invalid numbers", storageLimit.c_str());
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    } else if (digitStr.size() > 10) { // digitStr range is 0 ~ 4294967296, max length is 10
        MSPROF_LOGE("storage_limit:%s, valid range is 200~4294967296", storageLimit.c_str());
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }

    uint64_t limit = stoull(digitStr);
    if (limit < STORAGE_LIMIT_DOWN_THD) {
        MSPROF_LOGE("storage_limit:%s, min value is %uMB", storageLimit.c_str(), STORAGE_LIMIT_DOWN_THD);
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    } else if (limit > UINT32_MAX) {
        MSPROF_LOGE("storage_limit:%s, max value is %uMB", storageLimit.c_str(), UINT32_MAX);
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }
    return true;
}

bool ParamValidation::CheckTsCpuEventIsValid(const std::vector<std::string> &events)
{
    return CheckCtrlCpuEventIsValid(events);
}

bool ParamValidation::CheckCtrlCpuEventIsValid(const std::vector<std::string> &events)
{
    if (!CheckPmuEventSizeIsValid(events.size())) {
        MSPROF_LOGE("cpu events size(%u) is bigger than %d", events.size(), MAX_EVENT_SIZE);
        return false;
    }
    for (uint32_t i = 0; i < events.size(); i++) {
        std::string eventStr = analysis::dvvp::common::utils::Utils::ToLower(events[i]);
        if (eventStr.find("0x") == std::string::npos) {
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckLlcEventsIsValid(const std::string &events)
{
    for (auto ch : events) {
        if (ch == '_' || ch == '/' || (ch == ',' || ch == ' ')) {
            continue;
        } else if ((ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A')) {
            continue;
        } else if ((ch <= '9' && ch >= '0')) {
            continue;
        } else {
            MSPROF_LOGE("llc events is invalid.%s", events.c_str());
            return false;
        }
    }
    MSPROF_LOGD("llc events is %s", events.c_str());
    return true;
}

bool ParamValidation::CheckProfilingAicoreMetricsIsValid(const std::string &aicoreMetrics)
{
    if (aicoreMetrics.empty()) {
        MSPROF_LOGI("aicoreMetrics is empty");
        return true;
    }

    const std::vector<std::string> aicoreMetricsWhiteList = {
        ARITHMETIC_UTILIZATION,
        PIPE_UTILIZATION,
        MEMORY_BANDWIDTH,
        L0B_AND_WIDTH,
        RESOURCE_CONFLICT_RATIO,
        MEMORY_UB
    };

    for (size_t j = 0; j < aicoreMetricsWhiteList.size(); j++) {
        if (aicoreMetrics.compare(aicoreMetricsWhiteList[j]) == 0) {
            MSPROF_LOGD("aicoreMetrics is %s", aicoreMetrics.c_str());
            return true;
        }
    }

    MSPROF_LOGE("aicoreMetrics[%s] is invalid", aicoreMetrics.c_str());
    return false;
}

bool ParamValidation::CheckLlcModeIsValid(const std::string &llcMode)
{
    std::vector<std::string> llcModeWhiteList = {
        "read",
        "write"
    };
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        llcModeWhiteList = {"capacity", "bandwidth"};
    }
    if (llcMode.empty()) {
        MSPROF_LOGI("llcMode is empty");
        return true;
    }
    for (size_t i = 0; i < llcModeWhiteList.size(); i++) {
        if (llcMode.compare(llcModeWhiteList[i]) == 0) {
            return true;
        }
    }
    return false;
}


bool ParamValidation::CheckFreqIsValid(const std::string &freq, const int rangeMin, const int rangeMax)
{
    if (freq.empty()) {
        MSPROF_LOGI("freq is empty");
        return true;
    }
    if (Utils::CheckStringIsNonNegativeIntNum(freq)) {
        int optRet = std::stoi(freq);
        if ((optRet >= rangeMin) && (optRet <= rangeMax)) {
            return true;
        } else {
            MSPROF_LOGE("Input freq value is out of value range.");
            return false;
        }
    } else {
        MSPROF_LOGE("Input freq value is invalid.");
        return false;
    }
}

bool ParamValidation::CheckHostSysUsageIsValid(const std::string &hostSysUsage)
{
    if (Platform::instance()->RunSocSide()) {
        MSPROF_LOGE("Not in host side, host-sys-usage is not supported");
        return false;
    }
    if (hostSysUsage.empty()) {
        MSPROF_LOGI("hostsysusage is empty");
        return true;
    }
    std::vector<std::string> hostSysUsageArray = Utils::Split(hostSysUsage, false, "", ",");
    for (size_t i = 0; i < hostSysUsageArray.size(); ++i) {
        if ((hostSysUsageArray[i].compare("cpu") != 0) || (hostSysUsageArray[i].compare("mem") != 0)) {
            MSPROF_LOGE("Argument host-sys-usage: invalid value:%s. Please input in the range of "
                "'cpu | mem'", hostSysUsageArray[i].c_str());
            return false;
        }   
    }
    return true;
}

bool ParamValidation::CheckHostSysPidValid(const std::string &hostSysPid)
{
    if (hostSysPid.empty()) {
        MSPROF_LOGE("Argument --host-sys-pid: expected one argument");
        return false;
    }
    if (Utils::CheckStringIsNonNegativeIntNum(hostSysPid)) {
        auto hostSysRet = std::stoi(hostSysPid);
        if (!CheckHostSysPidIsValid(hostSysRet)) {
            MSPROF_LOGE("Argument --host-sys-pid: invalid int value: %d."
                "The process cannot be found, please enter a correct host-sys-pid.", hostSysRet);
            return false;
        }
        return true;
    } else {
        MSPROF_LOGE("Argument --host-sys-pid: invalid value: %s."
            "Please input an integer value.The min value is 0.", hostSysPid.c_str());
        return false;
    }
}

bool ParamValidation::CheckPythonPathIsValid(const std::string&pythonPath)
{
    if (pythonPath.empty()) {
        MSPROF_LOGE("Argument --python-path: expected one argument");
        return false;
    }
    if (pythonPath.size() > MAX_PATH_LENGTH) {
        MSPROF_LOGE("Argument --python-path is invalid because of exceeds"
            " the maximum length of %d", MAX_PATH_LENGTH);
        return false;
    }
    std::string absolutePythonPath = Utils::CanonicalizePath(pythonPath);
    if (absolutePythonPath.empty()) {
        MSPROF_LOGE("Argument --python-path %s does not exist or permission denied!!!", pythonPath.c_str());
        return false;
    }
    if (MmAccess2(absolutePythonPath, M_X_OK) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Argument --python-path %s permission denied.", pythonPath.c_str());
        return false;
    }
    if (Utils::IsDir(absolutePythonPath)) {
        MSPROF_LOGE("Argument --python-path %s is a directory, "
            "please enter the executable file path.", pythonPath.c_str());
        return false;
    }
    return true;
}

bool ParamValidation::CheckExportSummaryFormatIsValid(const std::string &summaryFormat)
{
    if (summaryFormat.empty()) {
        MSPROF_LOGE("Argument --summary-format: expected one argument");
        return false;
    }
    if (summaryFormat != "json" && summaryFormat!= "csv") {
        MSPROF_LOGE("Argument --summary-format: invalid value: %s. "
            "Please input 'json' or 'csv'.", summaryFormat.c_str());
        return false;
    }
    return true;
}

bool ParamValidation::CheckExportIdIsValid(const std::string &exportId, const std::string &exportIdType)
{
    if (exportId.empty()) {
        MSPROF_LOGE("Argument --%s: expected one argument", exportIdType.c_str());
        return false;
    }
    if (!Utils::CheckStringIsValidNatureNum(exportId)) {
        MSPROF_LOGE("Argument --%s: invalid value: %s."
            "Please input an integer value.", exportIdType.c_str(), exportId.c_str());
        return false;
    }
    return true;
}

bool ParamValidation::CheckL2CacheEventsValid(const std::vector<std::string> &events)
{
    if (!CheckPmuEventSizeIsValid(events.size())) {
        MSPROF_LOGE("L2Cache events size(%u) is bigger than %d", events.size(), MAX_EVENT_SIZE);
        return false;
    }
    const std::vector<std::string> l2CacheTaskEventsWhiteList = {
        "0x59", "0x5b", "0x5c", "0x62", "0x6a", "0x6c", "0x71",
        "0x74", "0x77", "0x78", "0x79", "0x7c", "0x7d", "0x7e"
    };
    for (uint32_t i = 0; i < events.size(); i++) {
        MSPROF_LOGD("l2Cacheevents:%s", events[i].c_str());
        bool findFlag = false;
        std::string event = analysis::dvvp::common::utils::Utils::ToLower(events[i]);
        event = analysis::dvvp::common::utils::Utils::Trim(event);
        for (std::string temp : l2CacheTaskEventsWhiteList) {
            if (temp == event) {
                MSPROF_LOGD("l2Cacheevent:%s", event.c_str());
                findFlag = true;
                break;
            }
        }
        if (!findFlag) {
            MSPROF_LOGE("l2Cacheevent:%s is invalid", event.c_str());
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckProfilingIntervalIsValidTWO(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params == nullptr) {
        MSPROF_LOGE("[CheckProfilingIntervalIsValidTWO]params is null");
        return false;
    }
    if (!IsValidInterval(params->sys_sampling_interval, "sys_profiling")) {
        return false;
    }
    if (!IsValidInterval(params->pid_sampling_interval, "pid_profiling")) {
        return false;
    }
    if (!IsValidInterval(params->hardware_mem_sampling_interval, "hardware_mem")) {
        return false;
    }
    if (!IsValidInterval(params->io_sampling_interval, "io_profiling")) {
        return false;
    }
    if (!IsValidInterval(params->interconnection_sampling_interval, "interconnection_profiling")) {
        return false;
    }
    if (!IsValidInterval(params->dvpp_sampling_interval, "dvpp_profiling")) {
        return false;
    }
    if (!IsValidInterval(params->nicInterval, "nicProfiling")) {
        return false;
    }
    if (!IsValidInterval(params->roceInterval, "roceProfiling")) {
        return false;
    }
    if (!IsValidInterval(params->aicore_sampling_interval, "aicore_profiling")) {
        return false;
    }
    return true;
}

bool ParamValidation::CheckProfilingIntervalIsValid(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params == nullptr) {
        return false;
    }
    if (!IsValidInterval(params->aiv_sampling_interval, "aiv")) {
        return false;
    }
    if (!IsValidInterval(params->hccsInterval, "hccl")) {
        return false;
    }
    if (!IsValidInterval(params->pcieInterval, "pcie")) {
        return false;
    }
    if (!IsValidInterval(params->roceInterval, "roce")) {
        return false;
    }
    if (!IsValidInterval(params->llc_interval, "llc")) {
        return false;
    }
    if (!IsValidInterval(params->ddr_interval, "ddr")) {
        return false;
    }
    if (!IsValidInterval(params->hbmInterval, "hbm")) {
        return false;
    }
    if (!IsValidInterval(params->cpu_sampling_interval, "cpu_profiling")) {
        return false;
    }
    return CheckProfilingIntervalIsValidTWO(params);
}

bool ParamValidation::CheckSystemTraceSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params == nullptr) {
        MSPROF_LOGE("[CheckSystemTraceSwitchProfiling]params is null");
        return false;
    }
    if (!IsValidSwitch(params->cpu_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->tsCpuProfiling)) {
        return false;
    }
    if (!IsValidSwitch(params->aiCtrlCpuProfiling)) {
        return false;
    }
    if (!IsValidSwitch(params->sys_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->pid_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->hardware_mem)) {
        return false;
    }
    if (!IsValidSwitch(params->io_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->interconnection_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->dvpp_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->nicProfiling)) {
        return false;
    }
    if (!IsValidSwitch(params->roceProfiling)) {
        return false;
    }
    return true;
}

bool ParamValidation::CheckProfilingSwitchIsValid(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params == nullptr) {
        return false;
    }
    if (!CheckTsSwitchProfiling(params)) {
        return false;
    }
    if (!CheckPmuSwitchProfiling(params)) {
        return false;
    }
    if (!CheckSystemTraceSwitchProfiling(params)) {
        return false;
    }
    return true;
}

bool ParamValidation::IsValidSwitch(const std::string &switchStr)
{
    if (switchStr.empty()) {
        return true;
    }
    return switchStr.compare("on") == 0 ||
        switchStr.compare("off") == 0;
}

bool ParamValidation::CheckTsSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (!IsValidSwitch(params->ts_task_track)) {
        return false;
    }
    if (!IsValidSwitch(params->ts_cpu_usage)) {
        return false;
    }
    if (!IsValidSwitch(params->ai_core_status)) {
        return false;
    }
    if (!IsValidSwitch(params->ts_timeline)) {
        return false;
    }
    if (!IsValidSwitch(params->ts_keypoint)) {
        return false;
    }
    if (!IsValidSwitch(params->ai_vector_status)) {
        return false;
    }
    if (!IsValidSwitch(params->ts_fw_training)) {
        return false;
    }
    if (!IsValidSwitch(params->hwts_log)) {
        return false;
    }
    if (!IsValidSwitch(params->hwts_log1)) {
        return false;
    }
    if (!IsValidSwitch(params->ts_memcpy)) {
        return false;
    }
    return true;
}
bool ParamValidation::CheckPmuSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (!IsValidSwitch(params->ai_core_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->aiv_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->ddr_profiling)) {
        return false;
    }
    if (!IsValidSwitch(params->hccsProfiling)) {
        return false;
    }
    if (!IsValidSwitch(params->pcieProfiling)) {
        return false;
    }
    if (!IsValidSwitch(params->hbmProfiling)) {
        return false;
    }
    return true;
}

bool ParamValidation::CheckDeviceIdIsValid(const std::string &devId)
{
    if (!analysis::dvvp::common::utils::Utils::CheckStringIsNonNegativeIntNum(devId)) {
        MSPROF_LOGE("devId(%s) is not valid.", devId.c_str());
        return false;
    }
    if (std::stoi(devId) >= 64) { // 64 : devIdMaxNum
        MSPROF_LOGE("devId(%s) is over limited.", devId.c_str());
        return false;
    }
    return true;
}

bool ParamValidation::CheckAiCoreEventsIsValid(const std::vector<std::string> &events)
{
    if (!CheckPmuEventSizeIsValid(events.size())) {
        MSPROF_LOGE("ai core events size(%u) is bigger than %d", events.size(), MAX_EVENT_SIZE);
        return false;
    }
    const int minEvent = 1;  // min ai core event is 0x1
    const int maxEvent = 135;  // max ai core event is 0x86
    for (unsigned int i = 0; i < events.size(); ++i) {
        int eventVal = strtol(events[i].c_str(), nullptr, BASE_HEX);
        if (eventVal < minEvent || eventVal > maxEvent) {
            MSPROF_LOGE("ai core event(%s) is not valid!", events[i].c_str());
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckPmuEventSizeIsValid(const int eventSize)
{
    MSPROF_LOGD("eventSize: %d", eventSize);
    return eventSize <= MAX_EVENT_SIZE;
}

bool ParamValidation::CheckCoreIdSizeIsValid(const int eventSize)
{
    return eventSize <= MAX_CORE_ID_SIZE;
}

bool ParamValidation::CheckAiCoreEventCoresIsValid(const std::vector<int> &coreId)
{
    if (!CheckCoreIdSizeIsValid(coreId.size())) {
        MSPROF_LOGE("ai core events cores size(%u) is bigger than %d", coreId.size(), MAX_EVENT_SIZE);
        return false;
    }
    for (unsigned int i = 0; i < coreId.size(); ++i) {
        if (coreId[i] < 0) {
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckAivEventCoresIsValid(const std::vector<int> &coreId)
{
    return CheckAiCoreEventCoresIsValid(coreId);
}

bool ParamValidation::CheckDdrEventsIsValid(const std::vector<std::string> &events)
{
    if (!CheckPmuEventSizeIsValid(events.size())) {
        MSPROF_LOGE("ddr events size(%u) is bigger than %d", events.size(), MAX_EVENT_SIZE);
        return false;
    }
    for (unsigned int i = 0; i < events.size(); ++i) {
        if (events[i].compare("master_id") == 0) {
            return true;
        }
    }
    return CheckHbmEventsIsValid(events);  // same with hbm event
}

bool ParamValidation::ParamValidation::CheckHbmEventsIsValid(const std::vector<std::string> &events)
{
    if (!CheckPmuEventSizeIsValid(events.size())) {
        MSPROF_LOGE("hbm events size(%u) is bigger than %d", events.size(), MAX_EVENT_SIZE);
        return false;
    }
    for (unsigned int i = 0; i < events.size(); ++i) {
        if (events[i].compare("read") != 0 && events[i].compare("write") != 0) {
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckAivEventsIsValid(const std::vector<std::string> &events)
{
    return CheckAiCoreEventsIsValid(events);  // same with ai core events
}

bool ParamValidation::CheckNameContainsDangerCharacter(const std::string &cmd)
{
    const std::string dangerList[] = {"rm ", "mv ", "reboot", "shutdown", "halt", "> ", "wget ", "poweroff"};
    for (std::string str: dangerList) {
        if (cmd.find(str) != std::string::npos) {
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckAppNameIsValid(const std::string &appName)
{
    if (appName.empty()) {
        MSPROF_LOGE("appName is empty");
        return false;
    }
    for (auto ch : appName) {
        if (ch == '_' || ch == '-' || ch == '.') {
            continue;
        } else if ((ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A')) {
            continue;
        } else if ((ch <= '9' && ch >= '0')) {
            continue;
        } else {
            MSPROF_LOGE("appName is invalid.%s", appName.c_str());
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckDataTagIsValid(const std::string &tag) const
{
    if (tag.empty()) {
        MSPROF_LOGE("tag is empty");
        return false;
    }
    for (auto ch : tag) {
        if (ch == '_') {
            continue;
        } else if ((ch <= 'z' && ch >= 'a') || (ch <= 'Z' && ch >= 'A')) {
            continue;
        } else if ((ch <= '9' && ch >= '0')) {
            continue;
        } else {
            MSPROF_LOGE("tag is invalid.%s", tag.c_str());
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckParamsDevices(const std::string &app, const std::string &paramsDevices,
                                         const std::string &paramsHostSys)
{
    if (paramsDevices.empty() && app.empty()) {
        // If only host data is collected, the device parameter is not required.
        if (!paramsHostSys.empty()) {
            return true;
        } else {
            return false;
        }
    }

    if (paramsDevices == "all") {
        return true;
    }

    bool result = true;
    std::vector<std::string> devices =
        analysis::dvvp::common::utils::Utils::Split(paramsDevices, false, "", ",");
    for (size_t i = 0; i < devices.size(); ++i) {
        if (!ParamValidation::instance()->CheckDeviceIdIsValid(devices[i])) {
            MSPROF_LOGE("device:%s is not valid!", devices[i].c_str());
            result = false;
            continue; // find all invalid values for device id
        }
    }
    return result;
}

bool ParamValidation::CheckParamsJobIdRegexMatch(const std::string &paramsJobId)
{
    size_t jobIdStrMaxLength = 512; // 512 : max length of joid
    if (paramsJobId.empty() || paramsJobId.length() > jobIdStrMaxLength) {
        return false;
    }
    for (size_t i = 0; i < paramsJobId.length(); i++) {
        if (paramsJobId[i] >= '0' && paramsJobId[i] <= '9') {
            continue;
        } else if (paramsJobId[i] >= 'a' && paramsJobId[i] <= 'z') {
            continue;
        } else if (paramsJobId[i] >= 'A' && paramsJobId[i] <= 'Z') {
            continue;
        } else if (paramsJobId[i] == '-') {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckSamplingFreq(std::string freq, int minVal, int maxVal)
{
    if (Utils::CheckStringIsNonNegativeIntNum(freq)) {
        int optRet = std::stoi(freq);
        if ((optRet >= minVal) && (optRet <= maxVal)) {
            return true;
        }
    }
    MSPROF_LOGE("frequency: %s set failed, it should be in range of (%d, %d)", freq.c_str(), minVal, maxVal);
    return false;
}

bool ParamValidation::CheckParamsModeRegexMatch(const std::string &paramsMode)
{
    if (paramsMode.empty()) {
        return true;
    }
    std::vector<std::string> profilingModeWhiteList;
    profilingModeWhiteList.push_back(analysis::dvvp::message::PROFILING_MODE_DEF);
    profilingModeWhiteList.push_back(analysis::dvvp::message::PROFILING_MODE_SYSTEM_WIDE);

    for (size_t i = 0; i < profilingModeWhiteList.size(); i++) {
        if (paramsMode.compare(profilingModeWhiteList[i]) == 0) {
            return true;
        } else {
            continue;
        }
    }
    return false;
}

bool ParamValidation::CheckProfilingParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params)
{
    if (params == nullptr) {
        MSPROF_LOGE("[CheckProfilingParams]params is nullptr.");
        return false;
    }

    if (!(params->host_profiling) && !(CheckParamsDevices(params->app, params->devices, params->host_sys))) {
        MSPROF_LOGE("[CheckProfilingParams]devices:%s is illegal", params->devices.c_str());
        return false;
    }
    SHARED_PTR_ALIA<std::vector<std::string>> l2CacheTaskProfilingEvents;
    MSVP_MAKE_SHARED0_RET(l2CacheTaskProfilingEvents, std::vector<std::string>, PROFILING_FAILED);
    *l2CacheTaskProfilingEvents = analysis::dvvp::common::utils::Utils::Split(
        params->l2CacheTaskProfilingEvents, false, "", ",");
    if (!CheckL2CacheEventsValid(*l2CacheTaskProfilingEvents)) {
        MSPROF_LOGE("[CheckProfilingParams]l2CacheTaskProfilingEvents is illegal");
        return false;
    }
    if (!CheckLlcEventsIsValid(params->llc_profiling_events)) {
        MSPROF_LOGE("[CheckProfilingParams]llc_profiling_events is illegal");
        return false;
    }
    if (!CheckProfilingIntervalIsValid(params)) {
        MSPROF_LOGE("[CheckProfilingParams]profiling interval is illegal");
        return false;
    }
    if (!CheckProfilingSwitchIsValid(params)) {
        MSPROF_LOGE("[CheckProfilingParams]profiling switch is illegal");
        return false;
    }
    if (!CheckProfilingAicoreMetricsIsValid(params->ai_core_metrics)) {
        MSPROF_LOGE("[CheckProfilingParams]profiling ai_core_metrics is illegal");
        return false;
    }
    return true;
}

int ParamValidation::CheckEventsSize(const std::string &events) const
{
    if (events.empty()) {
        MSPROF_LOGI("events is empty");
        return PROFILING_SUCCESS;
    }
    const std::vector<std::string> eventsList =
        analysis::dvvp::common::utils::Utils::Split(events, false, "", ",");
    const int eventsListMaxSize = 8;  // 8 is max event list len
    if (eventsList.size() > eventsListMaxSize) {
        MSPROF_LOGE("events Size is incorrect. %s", events.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

bool ParamValidation::IsValidInterval(const int interval, const std::string &logKey)
{
    if (interval < MIN_INTERVAL || interval > MAX_INTERVAL) {
        MSPROF_LOGE("invalid %s interval: %d", logKey.c_str(), interval);
        return false;
    }
    return true;
}

bool ParamValidation::IsValidSleepPeriod(const int period)
{
    if (period < MIN_INTERVAL || period > MAX_PERIOD) {
        MSPROF_LOGE("Invalid --sys-period: %d", period);
        return false;
    }
    return true;
}

bool ParamValidation::CheckHostSysOptionsIsValid(const std::string &hostSysOptions)
{
    if (hostSysOptions.empty()) {
        MSPROF_LOGI("hostSysOptions is empty");
        return false;
    }

    const std::vector<std::string> hostSysWhiteList = {
        HOST_SYS_CPU,
        HOST_SYS_MEM,
        HOST_SYS_DISK,
        HOST_SYS_NETWORK,
        HOST_SYS_OSRT
    };

    for (size_t j = 0; j < hostSysWhiteList.size(); j++) {
        if (hostSysOptions.compare(hostSysWhiteList[j]) == 0) {
            MSPROF_LOGD("hostSysOptions is %s", hostSysOptions.c_str());
            return true;
        }
    }

    MSPROF_LOGE("hostSysOptions[%s] is invalid", hostSysOptions.c_str());
    return false;
}

bool ParamValidation::CheckHostSysPidIsValid(const int hostSysPid)
{
    if (hostSysPid < 0) {
        MSPROF_LOGE("Invalid --host-sys-pid: %d", hostSysPid);
        return false;
    }

    std::string hostPidPath = "/proc/" + std::to_string(hostSysPid) + "/status";
    mmStat_t statBuf;
    int ret = MmStatGet(hostPidPath, &statBuf);
    if (ret < 0) {
        MSPROF_LOGE("Invalid --host-sys-pid: %d", hostSysPid);
        return false;
    }
    return true;
}

bool ParamValidation::ProfStarsAcsqParamIsValid(const std::string &param)
{
    if (param.empty()) {
        return true;
    }

    std::vector<std::string> starsAcsqParamVec = {
        "dsa", "vdec", "jpegd", "jpege", "vpc", "topic", "pcie", "rocee", "sdma", "ctrl_task"
    };

    auto paramVec = analysis::dvvp::common::utils::Utils::Split(param, false, "", ",");
    for (size_t i = 0; i < paramVec.size(); i++) {
        auto iter = find(starsAcsqParamVec.begin(), starsAcsqParamVec.end(), paramVec.at(i));
        if (iter == starsAcsqParamVec.end()) {
            MSPROF_LOGE("Invalid stars acsq params.");
            return false;
        }
    }
    return true;
}

bool ParamValidation::CheckStorageLimit(const std::string &storageLimit)
{
    if (storageLimit.empty()) {
        MSPROF_LOGI("storage_limit is empty");
        return true;
    }
    std::string errReason = "storage-limit should be in range [" + std::to_string(STORAGE_LIMIT_DOWN_THD) +
        "," + std::to_string(UINT32_MAX) + "], end with MB";

    uint32_t unitLen = strlen(STORAGE_LIMIT_UNIT);
    if (storageLimit.size() <= unitLen) {
        MSPROF_LOGE("storage_limit:%s, length is less than %u", storageLimit.c_str(), unitLen + 1);
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }

    std::string unitStr = storageLimit.substr(storageLimit.size() - unitLen);
    if (unitStr != STORAGE_LIMIT_UNIT) {
        MSPROF_LOGE("storage_limit:%s, not end with MB", storageLimit.c_str());
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }

    std::string digitStr = storageLimit.substr(0, storageLimit.size() - unitLen);
    if (!Utils::IsAllDigit(digitStr)) {
        MSPROF_LOGE("storage_limit:%s, invalid numbers", storageLimit.c_str());
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    } else if (digitStr.size() > 10) { // digitStr range is 0 ~ 4294967296, max length is 10
        MSPROF_LOGE("storage_limit:%s, valid range is 200~4294967296", storageLimit.c_str());
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }

    uint64_t limit = stoull(digitStr);
    if (limit < STORAGE_LIMIT_DOWN_THD) {
        MSPROF_LOGE("storage_limit:%s, min value is %uMB", storageLimit.c_str(), STORAGE_LIMIT_DOWN_THD);
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    } else if (limit > UINT32_MAX) {
        MSPROF_LOGE("storage_limit:%s, max value is %uMB", storageLimit.c_str(), UINT32_MAX);
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"storage_limit", storageLimit, errReason}));
        return false;
    }
    return true;
}

bool ParamValidation::CheckBiuFreqValid(const uint32_t biuFreq)
{
    if ((biuFreq < BIU_SAMPLE_FREQ_MIN) || (biuFreq > BIU_SAMPLE_FREQ_MAX)) {
        MSPROF_LOGE("biu_freq %u is invalid (%u~%u).", biuFreq, BIU_SAMPLE_FREQ_MIN, BIU_SAMPLE_FREQ_MAX);
        std::string errReason = "biu_freq should be in range [" + std::to_string(BIU_SAMPLE_FREQ_MIN) +
            "," + std::to_string(BIU_SAMPLE_FREQ_MAX) + "]";
        MSPROF_INPUT_ERROR("EK0003", std::vector<std::string>({"config", "value", "reason"}),
            std::vector<std::string>({"biu_freq", std::to_string(biuFreq), errReason}));
        return false;
    }
    return true;
}
}
}
}
}
