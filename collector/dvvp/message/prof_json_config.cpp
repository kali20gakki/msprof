/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: handle profiling request
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2024-01-23
 */

#include "prof_json_config.h"
#include "msprof_dlog.h"
#include "nlohmann/json.hpp"
#include "errno/error_code.h"

namespace analysis {
namespace dvvp {
namespace message {
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::config;

const std::vector<std::string> geCfgName = {"dvpp_freq", "host_sys_usage_freq", "instr_profiling_freq",
                                            "sys_hardware_mem_freq", "sys_interconnection_freq",
                                            "sys_io_sampling_freq", "aic_metrics", "aicpu", "aiv_metrics",
                                            "bp_point", "fp_point", "hccl", "host_sys", "host_sys_usage",
                                            "l2", "llc_profiling", "msproftx", "output", "power", "runtime_api",
                                            "storage_limit", "task_time", "task_trace", "training_trace"};
const std::vector<std::string> aclCfgName = {"dvpp_freq", "host_sys_usage_freq", "instr_profiling_freq",
                                             "sys_hardware_mem_freq", "sys_interconnection_freq",
                                             "sys_io_sampling_freq", "aic_metrics", "aicpu", "aiv_metrics",
                                             "ascendcl", "hccl", "host_sys", "host_sys_usage",
                                             "l2", "llc_profiling", "msproftx", "output", "runtime_api",
                                             "storage_limit", "task_time", "switch"};

inline bool isValueInArray(const std::vector<std::string>& arr, std::string value)
{
    for (int i = 0; i < arr.size(); i++) {
        if (arr[i] == value) {
            return true;
        }
    }
    return false;
}

int32_t JsonStringToGeCfg(const std::string &cfg, SHARED_PTR_ALIA<ProfGeOptionsConfig> &inputCfgPb)
{
    try {
        auto jsonFromCfg = nlohmann::json::parse(cfg);
        for (const auto &item: jsonFromCfg.items()) {
            if (!isValueInArray(geCfgName, item.key())) {
                MSPROF_LOGE("Error: parsing invalid key:%s.", item.key().c_str());
                return PROFILING_FAILED;
            }
        }
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, dvppFreq, "dvpp_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, hostSysUsageFreq, "host_sys_usage_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, instrProfilingFreq, "instr_profiling_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, sysHardwareMemFreq, "sys_hardware_mem_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, sysInterconnectionFreq, "sys_interconnection_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, sysIoSamplingFreq, "sys_io_sampling_freq");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, aicMetrics, "aic_metrics");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, aicpu, "aicpu");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, aivMetrics, "aiv_metrics");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, bpPoint, "bp_point");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, fpPoint, "fp_point");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, hccl, "hccl");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, hostSys, "host_sys");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, hostSysUsage, "host_sys_usage");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, l2, "l2");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, llcProfiling, "llc_profiling");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, msproftx, "msproftx");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, output, "output");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, power, "power");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, runtimeApi, "runtime_api");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, storageLimit, "storage_limit");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, taskTime, "task_time");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, taskTrace, "task_trace");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, trainingTrace, "training_trace");
    } catch (std::exception &e) {
        MSPROF_LOGE("Error: parsing failed:%s", e.what());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int32_t JsonStringToAclCfg(const std::string &cfg, SHARED_PTR_ALIA<ProfAclConfig> &inputCfgPb)
{
    try {
        auto jsonFromCfg = nlohmann::json::parse(cfg);
        for (const auto &item: jsonFromCfg.items()) {
            if (!isValueInArray(aclCfgName, item.key())) {
                MSPROF_LOGE("Error: parsing invalid key:%s.", item.key().c_str());
                return PROFILING_FAILED;
            }
        }
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, dvppFreq, "dvpp_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, hostSysUsageFreq, "host_sys_usage_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, instrProfilingFreq, "instr_profiling_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, sysHardwareMemFreq, "sys_hardware_mem_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, sysInterconnectionFreq, "sys_interconnection_freq");
        GET_JSON_INT_VALUE(jsonFromCfg, inputCfgPb, sysIoSamplingFreq, "sys_io_sampling_freq");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, aicMetrics, "aic_metrics");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, aicpu, "aicpu");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, aivMetrics, "aiv_metrics");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, ascendcl, "ascendcl");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, hccl, "hccl");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, hostSys, "host_sys");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, hostSysUsage, "host_sys_usage");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, l2, "l2");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, llcProfiling, "llc_profiling");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, msproftx, "msproftx");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, output, "output");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, runtimeApi, "runtime_api");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, storageLimit, "storage_limit");
        GET_JSON_STRING_VALUE(jsonFromCfg, inputCfgPb, taskTime, "task_time");
        if (jsonFromCfg.contains("switch")) {
            jsonFromCfg.at("switch").get_to(inputCfgPb->profSwitch);
        }
    } catch (std::exception &e) {
        MSPROF_LOGE("parse inputCfgPb faild:%s", e.what());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

}  // namespace message
}  // namespace dvvp
}  // namespace analysis