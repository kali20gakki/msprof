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

private:
    bool CheckTsSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckPmuSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool IsValidInterval(const int interval, const std::string &logKey) const;
    bool CheckProfilingIntervalIsValid(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckProfilingIntervalIsValidTWO(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
    bool CheckSystemTraceSwitchProfiling(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params);
};
}
}
}
}
#endif

