/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: handle profiling request
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2024-01-23
 */

#ifndef ANALYSIS_DVVP_MESSAGE_PROFILE_JSON_CONFIG_H
#define ANALYSIS_DVVP_MESSAGE_PROFILE_JSON_CONFIG_H

#include "message/message.h"
#include "utils/utils.h"

namespace analysis {
namespace dvvp {
namespace message {

struct ProfAclConfig {
    uint32_t dvppFreq;
    uint32_t hostSysUsageFreq;
    uint32_t instrProfilingFreq;
    uint32_t sysHardwareMemFreq;
    uint32_t sysInterconnectionFreq;
    uint32_t sysIoSamplingFreq;
    std::string aicMetrics;
    std::string aicpu;
    std::string aivMetrics;
    std::string ascendcl;
    std::string hccl;
    std::string hostSys;
    std::string hostSysUsage;
    std::string l2;
    std::string llcProfiling;
    std::string msproftx;
    std::string output;
    std::string runtimeApi;
    std::string storageLimit;
    std::string profSwitch;
    std::string taskTime;
    ProfAclConfig()
        : dvppFreq(0), hostSysUsageFreq(0), instrProfilingFreq(0), sysHardwareMemFreq(0),
          sysInterconnectionFreq(0), sysIoSamplingFreq(0), aicMetrics(""), aicpu(""), aivMetrics(""),
          ascendcl(""), hccl(""), hostSys(""), hostSysUsage(""), l2(""), llcProfiling(""),
          msproftx(""), output(""), runtimeApi(""), storageLimit(""), profSwitch(""), taskTime("") {}
};

struct ProfGeOptionsConfig {
    uint32_t dvppFreq;
    uint32_t hostSysUsageFreq;
    uint32_t instrProfilingFreq;
    uint32_t sysHardwareMemFreq;
    uint32_t sysInterconnectionFreq;
    uint32_t sysIoSamplingFreq;
    std::string aicMetrics;
    std::string aicpu;
    std::string aivMetrics;
    std::string bpPoint;
    std::string fpPoint;
    std::string hccl;
    std::string hostSys;
    std::string hostSysUsage;
    std::string l2;
    std::string llcProfiling;
    std::string msproftx;
    std::string output;
    std::string power;
    std::string runtimeApi;
    std::string storageLimit;
    std::string taskTime;
    std::string taskTrace;
    std::string trainingTrace;
    ProfGeOptionsConfig()
        : dvppFreq(0), hostSysUsageFreq(0), instrProfilingFreq(0), sysHardwareMemFreq(0),
          sysInterconnectionFreq(0), sysIoSamplingFreq(0), aicMetrics(""), aicpu(""), aivMetrics(""),
          bpPoint(""), fpPoint(""), hccl(""), hostSys(""), hostSysUsage(""), l2(""), llcProfiling(""), msproftx(""),
          output(""), power(""), runtimeApi(""), storageLimit(""), taskTime(""), taskTrace(""), trainingTrace("") {}
};

#define GET_JSON_INT_VALUE(json, dataStruct, field, inputSwitch)                                            \
    do {                                                                                                    \
        std::string intValueFromJson;                                                                       \
        uint32_t value;                                                                                     \
        if ((json).contains(inputSwitch)) {                                                                 \
            (json).at(inputSwitch).get_to(intValueFromJson);                                                \
            if (Utils::StrToUint32(value, intValueFromJson) == 0) { (dataStruct)->field = value; }          \
        }                                                                                                   \
    } while (0)

#define GET_JSON_STRING_VALUE(json, dataStruct, field, inputSwitch)         \
    do {                                                                    \
        if ((json).contains(inputSwitch)) {                                 \
            (json).at(inputSwitch).get_to((dataStruct)->field);             \
        }                                                                   \
    } while (0)

int32_t JsonStringToAclCfg(const std::string &cfg, SHARED_PTR_ALIA<ProfAclConfig> &inputCfgPb);
int32_t JsonStringToGeCfg(const std::string &cfg, SHARED_PTR_ALIA<ProfGeOptionsConfig> &inputCfgPb);

}  // namespace message
}  // namespace dvvp
}  // namespace analysis
#endif // ANALYSIS_DVVP_MESSAGE_PROFILE_JSON_CONFIG_H