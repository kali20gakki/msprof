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
#include "singleton/singleton.h"
#include "utils/utils.h"
#include "errno/error_code.h"
#include "mmpa_api.h"

namespace analysis {
namespace dvvp {
namespace common {
namespace validation {
class ParamValidation : public analysis::dvvp::common::singleton::Singleton<ParamValidation> {
public:
    ParamValidation();
    ~ParamValidation();

    int Init() const;
    int Uninit() const;
    bool CheckOutputIsValid(const std::string &outputPath);
    bool CheckStorageLimitIsValid(const std::string &storageLimit);
    bool CheckLlcModeIsValid(const std::string &llcMode);
    bool CheckFreqIsValid(const std::string &freq, const int rangeMin, const int rangeMax);
    bool CheckHostSysUsageIsValid(const std::string &hostSysUsage);
    bool CheckHostSysPidValid(const std::string &hostSysPid);
    bool CheckPythonPathIsValid(const std::string&pythonPath);
    bool CheckExportSummaryFormatIsValid(const std::string &summaryFormat);
    bool CheckExportIdIsValid(const std::string &exportId, const std::string &exportIdType);
    bool CheckProfilingParams(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckParamsDevices(const std::string &app, const std::string &paramsDevices,
        const std::string &paramsHostSys);
    bool CheckParamsJobIdRegexMatch(const std::string &paramsJobId);
    bool CheckParamsModeRegexMatch(const std::string &paramsMode);
    bool CheckL2CacheEventsValid(const std::vector<std::string> &events);
    bool CheckLlcEventsIsValid(const std::string &events);
    bool CheckProfilingSwitchIsValid(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckAiCoreEventsIsValid(const std::vector<std::string> &events);
    bool CheckAiCoreEventCoresIsValid(const std::vector<int> &coreId);
    bool CheckDdrEventsIsValid(const std::vector<std::string> &events);
    bool CheckHbmEventsIsValid(const std::vector<std::string> &events);
    bool CheckAivEventsIsValid(const std::vector<std::string> &events);
    bool CheckAivEventCoresIsValid(const std::vector<int> &coreId);
    bool CheckAppNameIsValid(const std::string &appName);
    bool CheckDataTagIsValid(const std::string &tag) const;
    bool CheckTsCpuEventIsValid(const std::vector<std::string> &events);
    bool CheckCtrlCpuEventIsValid(const std::vector<std::string> &events);
    bool CheckPmuEventSizeIsValid(const int eventSize);
    bool CheckCoreIdSizeIsValid(const int eventSize);
    bool CheckNameContainsDangerCharacter(const std::string &cmd);
    bool CheckDeviceIdIsValid(const std::string &devId);
    bool CheckProfilingAicoreMetricsIsValid(const std::string &aicoreMetrics);
    int CheckEventsSize(const std::string &events) const;
    bool IsValidSleepPeriod(const int period);
    bool CheckHostSysOptionsIsValid(const std::string &hostSysOptions);
    bool CheckHostSysPidIsValid(const int hostSysPid);
    bool ProfStarsAcsqParamIsValid(const std::string &param);
    bool IsValidSwitch(const std::string &switchStr);
    bool CheckStorageLimit(const std::string &storageLimit);
    bool CheckBiuFreqValid(const uint32_t biuFreq);

private:
    bool CheckTsSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckPmuSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool IsValidInterval(const int interval, const std::string &logKey);
    bool CheckProfilingIntervalIsValid(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckProfilingIntervalIsValidTWO(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckSystemTraceSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
};
}
}
}
}
#endif

