/**
* @file parameter_validation.h
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef ANALYSIS_DVVP_COMMON_PARAM_VALIDATION_H
#define ANALYSIS_DVVP_COMMON_PARAM_VALIDATION_H

#include <string>
#include "message/prof_params.h"
#include "errno/error_code.h"
#include "singleton/singleton.h"
#include "utils/utils.h"
#include "mmpa_api.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace validation {
const int FILE_FIND_REPLAY          = 100;
const std::string TOOL_NAME_PERF     = "perf";
const std::string TOOL_NAME_LTRACE   = "ltrace";
const std::string TOOL_NAME_IOTOP    = "iotop";
const int MIN_APP_LENTH_WITH_SCRIPT  = 2;
class ParamValidation : public analysis::dvvp::common::singleton::Singleton<ParamValidation> {
public:
    ParamValidation();
    ~ParamValidation();

    int Init() const;
    int Uninit() const;
    bool CheckOutputIsValid(const std::string &outputPath) const;
    bool CheckLlcModeIsValid(const std::string &llcMode) const;
    bool CheckFreqIsValid(const std::string &freq, const int rangeMin, const int rangeMax) const;
    bool CheckHostSysUsageIsValid(const std::string &hostSysUsage) const;
    bool CheckHostSysPidValid(const std::string &hostSysPid) const;
    bool CheckPythonPathIsValid(const std::string &pythonPath) const;
    bool CheckExportSummaryFormatIsValid(const std::string &summaryFormat) const;
    bool CheckExportIdIsValid(const std::string &exportId, const std::string &exportIdType) const;
    bool CheckProfilingParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckParamsDevices(const std::string &app, const std::string &paramsDevices,
        const std::string &paramsHostSys) const;
    bool CheckParamsJobIdRegexMatch(const std::string &paramsJobId) const;
    bool CheckParamsModeRegexMatch(const std::string &paramsMode) const;
    bool CheckL2CacheEventsValid(const std::vector<std::string> &events);
    bool CheckLlcEventsIsValid(const std::string &events) const;
    bool CheckProfilingSwitchIsValid(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckAiCoreEventsIsValid(const std::vector<std::string> &events);
    bool CheckAiCoreEventCoresIsValid(const std::vector<int> &coreId);
    bool CheckDdrEventsIsValid(const std::vector<std::string> &events);
    bool CheckHbmEventsIsValid(const std::vector<std::string> &events);
    bool CheckAivEventsIsValid(const std::vector<std::string> &events);
    bool CheckAivEventCoresIsValid(const std::vector<int> &coreId);
    bool CheckAppNameIsValid(const std::string &appName) const;
    bool CheckDataTagIsValid(const std::string &tag) const;
    bool CheckTsCpuEventIsValid(const std::vector<std::string> &events);
    bool CheckCtrlCpuEventIsValid(const std::vector<std::string> &events);
    bool CheckPmuEventSizeIsValid(const int eventSize) const;
    bool CheckCoreIdSizeIsValid(const int eventSize) const;
    bool CheckNameContainsDangerCharacter(const std::string &cmd) const;
    bool CheckDeviceIdIsValid(const std::string &devId) const;
    bool CheckProfilingAicoreMetricsIsValid(const std::string &aicoreMetrics) const;
    int CheckEventsSize(const std::string &events) const;
    bool IsValidSleepPeriod(const int period) const;
    bool CheckHostSysOptionsIsValid(const std::string &hostSysOptions) const;
    bool CheckHostSysPidIsValid(const int hostSysPid) const;
    bool ProfStarsAcsqParamIsValid(const std::string &param) const;
    bool IsValidSwitch(const std::string &switchStr) const;
    bool CheckStorageLimit(const std::string &storageLimit) const;
    bool CheckBiuFreqValid(const uint32_t biuFreq) const;
    bool MsprofCheckAppValid(std::string &appParam) const;
    bool MsprofCheckEnvValid(const std::string &envParam) const;
    bool MsprofCheckAiModeValid(const std::string &aiModeParam, const std::string &aiModeType) const;
    bool MsprofCheckSysDeviceValid(const std::string &devListParam) const;
    bool MsprofCheckSysPeriodValid(const std::string &sysPeriodParam) const;
    bool MsprofCheckHostSysValid(const std::string &hostSysParam) const;
    bool CheckHostSysToolsExit(const std::string &hostSysParam, const std::string &resultDir,
        const std::string &appDir) const;

private:
    bool CheckTsSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckPmuSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool IsValidInterval(const int interval, const std::string &logKey) const;
    bool CheckProfilingIntervalIsValid(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckProfilingIntervalIsValidTWO(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckSystemTraceSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    int MsprofCheckNotAppValid(const std::vector<std::string> &AppParamsList, std::string &resultAppParam) const;
    int MsprofCheckAppParamValid(const std::string &appParam) const;
    int MsprofCheckAppScriptValid(const std::vector<std::string> &appParams) const;
    int CheckHostSysToolsIsExist(const std::string toolName, const std::string &resultDir,
        const std::string &appDir) const;
    int CheckHostSysCmdOutIsExist(const std::string tmpDir, const std::string toolName,
                                           const MmProcess tmpProcess) const;
    int CheckHostOutString(const std::string tmpStr, const std::string toolName) const;
    int UninitCheckHostSysCmd(const MmProcess checkProcess) const;
};
}
}
}
}
#endif

