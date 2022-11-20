/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2021. All rights reserved.
 * Description: handle profiling request
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2018-06-13
 */
#ifndef ANALYSIS_DVVP_MESSAGE_PROFILE_PARAMS_H
#define ANALYSIS_DVVP_MESSAGE_PROFILE_PARAMS_H
#include <set>
#include <sstream>
#include "msprof_dlog.h"
#include "config/config.h"
#include "nlohmann/json.hpp"
#include "slog_plugin.h"
#include "utils/utils.h"
#include "message.h"

namespace analysis {
namespace dvvp {
namespace message {
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;
const int FALSE = 0;
const int TRUE = 1;
const char * const PROFILING_MODE_SAMPLE_BASED = "sample-based";
const char * const PROFILING_MODE_TASK_BASED = "task-based";
const char * const PROFILING_MODE_DEF = "def_mode";

// Attention:
// intervals of ProfileParams maybe large,
// remember to cast to long long before calculation (for example, convert ms to ns)
struct ProfileParams : BaseInfo {
    std::string job_id;
    std::string result_dir;
    std::string storageLimit;   // 数据老化阈值
    std::string profiling_mode;
    std::string devices;
    int msprofBinPid;
    int is_cancel;
    int profiling_period;
    std::string stream_enabled;
    std::string profiling_options;
    std::string jobInfo;
    // app
    std::string cmdPath;
    std::string app;
    std::string app_dir;
    std::string app_parameters;
    std::string app_env;
    // ai core
    std::string ai_core_profiling;
    int aicore_sampling_interval;
    std::string ai_core_profiling_mode;
    std::string ai_core_profiling_events;
    std::string ai_core_metrics;

    std::string aiv_profiling;
    int aiv_sampling_interval;
    std::string aiv_profiling_events;
    std::string aiv_metrics;
    std::string aiv_profiling_mode;

    // rts
    std::string ts_timeline;
    std::string ts_keypoint;
    std::string ts_memcpy;
    std::string ts_fw_training;
    std::string hwts_log;
    std::string hwts_log1;
    std::string stars_acsq_task;
    std::string low_power;
    std::string acc_pmu_mode;

    // system trace
    std::string cpu_profiling;
    int cpu_sampling_interval;
    // cpu
    std::string aiCtrlCpuProfiling;
    std::string ai_ctrl_cpu_profiling_events;
    std::string tsCpuProfiling;
    std::string ts_cpu_profiling_events;

    // sys mem, sys cpuusage, app mem, app cpuusage
    std::string sys_profiling;
    int sys_sampling_interval;
    std::string pid_profiling;
    int pid_sampling_interval;

    std::string hardware_mem;
    int hardware_mem_sampling_interval;
    std::string llc_profiling;
    std::string msprof_llc_profiling;
    std::string llc_profiling_events;
    int llc_interval;
    std::string ddr_profiling;
    std::string ddr_profiling_events;
    int ddr_interval;
    int ddr_master_id;
    std::string hbmProfiling;
    std::string hbm_profiling_events;
    int hbmInterval;
    std::string l2CacheTaskProfiling;
    std::string l2CacheTaskProfilingEvents;

    std::string io_profiling;
    int io_sampling_interval;
    std::string nicProfiling;
    int nicInterval;
    std::string roceProfiling;
    int roceInterval;

    std::string interconnection_profiling;
    int interconnection_sampling_interval;
    std::string hccsProfiling;
    int hccsInterval;
    std::string pcieProfiling;
    int pcieInterval;

    std::string dvpp_profiling;
    int dvpp_sampling_interval;

    std::string biu;
    int biu_freq;

    // for msprof
    std::string modelLoad;
    std::string msprof;
    std::string msproftx;

    // host sys
    std::string host_sys;
    int host_sys_pid;
    std::string host_sys_usage;
    std::string host_disk_profiling;
    std::string host_osrt_profiling;

    // app cpu/memory/network usage on host
    int host_profiling;
    std::string host_one_pid_cpu_profiling;
    std::string host_one_pid_mem_profiling;
    std::string host_all_pid_cpu_profiling;
    std::string host_all_pid_mem_profiling;
    std::string host_sys_cpu_profiling;
    std::string host_sys_mem_profiling;
    std::string host_network_profiling;
    int host_disk_freq;
    int host_cpu_profiling_sampling_interval;
    int host_mem_profiling_sampling_interval;

    // for parse, query and export
    std::string pythonPath;
    std::string parseSwitch;
    std::string querySwitch;
    std::string exportSwitch;
    std::string exportSummaryFormat;
    std::string exportIterationId;
    std::string exportModelId;

    // subset of MsprofArgsType
    std::set<int> usedParams;
    // AI STACK
    uint64_t dataTypeConfig;

    ProfileParams()
        : msprofBinPid(MSVP_MMPROCESS), is_cancel(FALSE), profiling_period(-1),
          stream_enabled("on"), profiling_options(""),
          aicore_sampling_interval(DEFAULT_PROFILING_INTERVAL_10MS),
          aiv_sampling_interval(DEFAULT_PROFILING_INTERVAL_10MS),
          cpu_profiling("off"), cpu_sampling_interval(DEFAULT_PROFILING_INTERVAL_20MS),
          aiCtrlCpuProfiling("off"), tsCpuProfiling("off"),
          sys_profiling("off"), sys_sampling_interval(DEFAULT_PROFILING_INTERVAL_100MS),
          pid_profiling("off"), pid_sampling_interval(DEFAULT_PROFILING_INTERVAL_100MS),
          hardware_mem("off"), hardware_mem_sampling_interval(DEFAULT_PROFILING_INTERVAL_20MS),
          llc_interval(DEFAULT_PROFILING_INTERVAL_20MS),
          ddr_interval(DEFAULT_PROFILING_INTERVAL_20MS), ddr_master_id(0),
          hbmInterval(DEFAULT_PROFILING_INTERVAL_20MS),
          io_profiling("off"), io_sampling_interval(DEFAULT_PROFILING_INTERVAL_10MS),
          nicProfiling("off"), nicInterval(DEFAULT_PROFILING_INTERVAL_10MS),
          roceProfiling("off"), roceInterval(DEFAULT_PROFILING_INTERVAL_10MS),
          interconnection_profiling("off"), interconnection_sampling_interval(DEFAULT_PROFILING_INTERVAL_20MS),
          hccsProfiling("off"), hccsInterval(DEFAULT_PROFILING_INTERVAL_20MS),
          pcieInterval(DEFAULT_PROFILING_INTERVAL_20MS),
          dvpp_profiling("off"), dvpp_sampling_interval(DEFAULT_PROFILING_INTERVAL_20MS),
          biu_freq(DEFAULT_PROFILING_BIU_FREQ),
          msprof("off"), msproftx("off"),
          host_sys(""), host_sys_pid(HOST_PID_DEFAULT), host_sys_usage(""),
          host_disk_profiling("off"), host_osrt_profiling("off"),
          host_profiling(FALSE), host_one_pid_cpu_profiling("off"), host_all_pid_cpu_profiling("off"),
          host_sys_cpu_profiling("off"), host_sys_mem_profiling("off"),
          host_one_pid_mem_profiling("off"), host_all_pid_mem_profiling("off"),
          host_network_profiling("off"), host_disk_freq(DEFAULT_PROFILING_INTERVAL_50MS),
          host_cpu_profiling_sampling_interval(DEFAULT_PROFILING_INTERVAL_20MS),
          host_mem_profiling_sampling_interval(DEFAULT_PROFILING_INTERVAL_20MS),
          pythonPath(""), parseSwitch("off"), querySwitch("off"), exportSwitch("off"),
          exportSummaryFormat(PROFILING_SUMMARY_FORMAT), exportIterationId(DEFAULT_INTERATION_ID),
          exportModelId(DEFAULT_MODEL_ID), usedParams(), dataTypeConfig(0)
    {
    }

    virtual ~ProfileParams() {}

    std::string GetStructName()
    {
        return "ProfileParams";
    }

    void PrintAllFields()
    {
        if (SlogPlugin::instance()->MsprofCheckLogLevelForC(MSPROF_MODULE_NAME, DLOG_INFO)) {
            nlohmann::json object;
            ToObject(object);
            std::stringstream ss;
            for (auto iter = object.begin(); iter != object.end(); iter++) {
                ss << iter.key() << ": ";
                if (iter.key() == "result_dir" || iter.key() == "app_dir") {
                    ss << "***/" << Utils::BaseName(iter.value());
                } else {
                    ss << iter.value();
                }
                MSPROF_LOGI("[PrintAllFields] %s", ss.str().c_str());
                ss.str("");
            }
        }
    }

    bool IsHostProfiling()
    {
        if (host_one_pid_cpu_profiling.compare("on") == 0 || host_one_pid_mem_profiling.compare("on") == 0 ||
            host_all_pid_cpu_profiling.compare("on") == 0 || host_all_pid_mem_profiling.compare("on") == 0 ||
            host_network_profiling.compare("on") == 0 || host_disk_profiling.compare("on") == 0 ||
            host_osrt_profiling.compare("on") == 0 ||  host_sys_cpu_profiling.compare("on") == 0 ||
            host_sys_mem_profiling.compare("on") == 0 || msproftx.compare("on") == 0) {
            return true;
        }
        return false;
    }

    bool IsMsprofTx() const
    {
        return msproftx == "on";
    }

    void ToObjectPartOne(nlohmann::json &object)
    {
        SET_VALUE(object, result_dir);
        SET_VALUE(object, storageLimit);
        SET_VALUE(object, profiling_mode);
        SET_VALUE(object, devices);
        SET_VALUE(object, ai_ctrl_cpu_profiling_events);
        SET_VALUE(object, ts_cpu_profiling_events);
        SET_VALUE(object, app_dir);
        SET_VALUE(object, app_parameters);
        SET_VALUE(object, app_env);
        SET_VALUE(object, cmdPath);
        // ai core
        SET_VALUE(object, ai_core_profiling);
        SET_VALUE(object, ai_core_profiling_mode);
        SET_VALUE(object, ai_core_profiling_events);
        SET_VALUE(object, ai_core_metrics);
        // aiv
        SET_VALUE(object, aiv_profiling);
        SET_VALUE(object, aiv_sampling_interval);
        SET_VALUE(object, aiv_profiling_events);
        SET_VALUE(object, aiv_metrics);
        SET_VALUE(object, aiv_profiling_mode);
        SET_VALUE(object, is_cancel);
        SET_VALUE(object, profiling_options);
        SET_VALUE(object, jobInfo);
        // system trace
        SET_VALUE(object, cpu_profiling);
        SET_VALUE(object, aiCtrlCpuProfiling);
        SET_VALUE(object, tsCpuProfiling);
        SET_VALUE(object, cpu_sampling_interval);
        SET_VALUE(object, sys_profiling);
        SET_VALUE(object, sys_sampling_interval);
        SET_VALUE(object, pid_profiling);
        SET_VALUE(object, pid_sampling_interval);
        SET_VALUE(object, hardware_mem);
        SET_VALUE(object, hardware_mem_sampling_interval);
        SET_VALUE(object, io_profiling);
        SET_VALUE(object, io_sampling_interval);
        SET_VALUE(object, interconnection_profiling);
        SET_VALUE(object, interconnection_sampling_interval);
        SET_VALUE(object, dvpp_profiling);
        SET_VALUE(object, nicProfiling);
        SET_VALUE(object, roceProfiling);
        // host system
        SET_VALUE(object, host_profiling);
        SET_VALUE(object, host_one_pid_cpu_profiling);
        SET_VALUE(object, host_all_pid_cpu_profiling);
        SET_VALUE(object, host_sys_cpu_profiling);
        SET_VALUE(object, host_one_pid_mem_profiling);
        SET_VALUE(object, host_all_pid_mem_profiling);
        SET_VALUE(object, host_sys_mem_profiling);
        SET_VALUE(object, host_network_profiling);
    }

    void ToObjectPartTwo(nlohmann::json &object)
    {
        SET_VALUE(object, dvpp_sampling_interval);
        SET_VALUE(object, aicore_sampling_interval);
        SET_VALUE(object, modelLoad);
        SET_VALUE(object, msprof);
        SET_VALUE(object, msproftx);
        SET_VALUE(object, job_id);
        SET_VALUE(object, app);
        SET_VALUE(object, ts_timeline);
        SET_VALUE(object, ts_memcpy);
        SET_VALUE(object, ts_keypoint);
        SET_VALUE(object, ts_fw_training);
        SET_VALUE(object, hwts_log);
        SET_VALUE(object, hwts_log1);
        SET_VALUE(object, l2CacheTaskProfiling);
        SET_VALUE(object, l2CacheTaskProfilingEvents);
        SET_VALUE(object, hccsProfiling);
        SET_VALUE(object, hccsInterval);
        SET_VALUE(object, pcieProfiling);
        SET_VALUE(object, pcieInterval);
        SET_VALUE(object, roceInterval);
        SET_VALUE(object, nicInterval);
        // llc
        SET_VALUE(object, llc_profiling);         // for analysis use, read, write, capacity
        SET_VALUE(object, msprof_llc_profiling); // for msprof self use, on or off
        SET_VALUE(object, llc_profiling_events);
        SET_VALUE(object, llc_interval);
        // biu
        SET_VALUE(object, biu);
        SET_VALUE(object, biu_freq);
        // ddr
        SET_VALUE(object, ddr_profiling);
        SET_VALUE(object, ddr_profiling_events);
        SET_VALUE(object, ddr_interval);
        SET_VALUE(object, ddr_master_id);
        // hbm
        SET_VALUE(object, hbmProfiling);
        SET_VALUE(object, hbm_profiling_events);
        SET_VALUE(object, hbmInterval);
        // for debug purpose
        SET_VALUE(object, stream_enabled);
        SET_VALUE(object, profiling_period);
        // host
        SET_VALUE(object, host_sys);
        SET_VALUE(object, host_sys_pid);
        SET_VALUE(object, host_sys_usage);
        SET_VALUE(object, host_disk_profiling);
        SET_VALUE(object, host_osrt_profiling);
        SET_VALUE(object, host_disk_freq);
        SET_VALUE(object, host_cpu_profiling_sampling_interval);
        SET_VALUE(object, host_mem_profiling_sampling_interval);
    }

    void ToObjectPartThree(nlohmann::json &object)
    {
        SET_VALUE(object, stars_acsq_task);
        SET_VALUE(object, low_power);
        SET_VALUE(object, acc_pmu_mode);
        SET_VALUE(object, msprofBinPid);
        SetUint64Value(object, MSG_STR(dataTypeConfig), dataTypeConfig);
    }

    void ToObject(nlohmann::json &object)
    {
        ToObjectPartOne(object);
        ToObjectPartTwo(object);
        ToObjectPartThree(object);
    }

    void FromObjectPartOne(const nlohmann::json &object)
    {
        FROM_STRING_VALUE(object, result_dir);
        FROM_STRING_VALUE(object, storageLimit);
        FROM_STRING_VALUE(object, profiling_mode);
        FROM_STRING_VALUE(object, devices);
        FROM_STRING_VALUE(object, ai_ctrl_cpu_profiling_events);
        FROM_STRING_VALUE(object, ts_cpu_profiling_events);
        FROM_STRING_VALUE(object, app_dir);
        FROM_STRING_VALUE(object, app_parameters);
        FROM_STRING_VALUE(object, app_env);
        FROM_STRING_VALUE(object, cmdPath);
        // ai core
        FROM_STRING_VALUE(object, ai_core_profiling);
        FROM_STRING_VALUE(object, ai_core_profiling_mode);
        FROM_STRING_VALUE(object, ai_core_profiling_events);
        FROM_STRING_VALUE(object, ai_core_metrics);
        // AIV
        FROM_STRING_VALUE(object, aiv_profiling);
        FROM_INT_VALUE(object, aiv_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_STRING_VALUE(object, aiv_metrics);
        FROM_STRING_VALUE(object, aiv_profiling_mode);
        FROM_STRING_VALUE(object, aiv_profiling_events);
        FROM_BOOL_VALUE(object, is_cancel);
        FROM_STRING_VALUE(object, profiling_options);
        FROM_STRING_VALUE(object, jobInfo);
        // system trace
        FROM_STRING_VALUE(object, cpu_profiling);
        FROM_STRING_VALUE(object, aiCtrlCpuProfiling);
        FROM_STRING_VALUE(object, tsCpuProfiling);
        FROM_INT_VALUE(object, cpu_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_STRING_VALUE(object, sys_profiling);
        FROM_INT_VALUE(object, sys_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_STRING_VALUE(object, pid_profiling);
        FROM_INT_VALUE(object, pid_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_STRING_VALUE(object, hardware_mem);
        FROM_INT_VALUE(object, hardware_mem_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_STRING_VALUE(object, io_profiling);
        FROM_INT_VALUE(object, io_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_STRING_VALUE(object, interconnection_profiling);
        FROM_INT_VALUE(object, interconnection_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_STRING_VALUE(object, dvpp_profiling);
        FROM_STRING_VALUE(object, nicProfiling);
        FROM_STRING_VALUE(object, roceProfiling);
        FROM_INT_VALUE(object, dvpp_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_INT_VALUE(object, aicore_sampling_interval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_INT_VALUE(object, biu_freq, DEFAULT_PROFILING_BIU_FREQ);
        // host system
        FROM_BOOL_VALUE(object, host_profiling);
        FROM_STRING_VALUE(object, host_one_pid_cpu_profiling);
        FROM_STRING_VALUE(object, host_all_pid_cpu_profiling);
    }

    void FromObjectPartTwo(const nlohmann::json &object)
    {
        FROM_STRING_VALUE(object, modelLoad);
        FROM_STRING_VALUE(object, msprof);
        FROM_STRING_VALUE(object, msproftx);
        FROM_STRING_VALUE(object, job_id);
        FROM_STRING_VALUE(object, app);
        FROM_STRING_VALUE(object, ts_timeline);
        FROM_STRING_VALUE(object, ts_memcpy);
        FROM_STRING_VALUE(object, ts_keypoint);
        FROM_STRING_VALUE(object, ts_fw_training);
        FROM_STRING_VALUE(object, hwts_log);
        FROM_STRING_VALUE(object, hwts_log1);
        FROM_STRING_VALUE(object, stars_acsq_task);
        FROM_STRING_VALUE(object, low_power);
        FROM_STRING_VALUE(object, acc_pmu_mode);
        FROM_STRING_VALUE(object, l2CacheTaskProfiling);
        FROM_STRING_VALUE(object, l2CacheTaskProfilingEvents);
        FROM_STRING_VALUE(object, hccsProfiling);
        FROM_INT_VALUE(object, hccsInterval, DEFAULT_PROFILING_INTERVAL_100MS);
        FROM_STRING_VALUE(object, pcieProfiling);
        FROM_INT_VALUE(object, pcieInterval, DEFAULT_PROFILING_INTERVAL_100MS);
        FROM_INT_VALUE(object, roceInterval, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_INT_VALUE(object, nicInterval, DEFAULT_PROFILING_INTERVAL_10MS);
        // llc
        FROM_STRING_VALUE(object, llc_profiling);
        FROM_STRING_VALUE(object, msprof_llc_profiling);
        FROM_STRING_VALUE(object, llc_profiling_events);
        FROM_INT_VALUE(object, llc_interval, DEFAULT_PROFILING_INTERVAL_100MS);
    }

    void FromObjectPartThree(const nlohmann::json &object)
    {
        FROM_INT_VALUE(object, msprofBinPid, MSVP_MMPROCESS);
        // ddr
        FROM_STRING_VALUE(object, ddr_profiling);
        FROM_STRING_VALUE(object, ddr_profiling_events);
        FROM_INT_VALUE(object, ddr_interval, DEFAULT_PROFILING_INTERVAL_100MS);
        FROM_INT_VALUE(object, ddr_master_id, 0);
        // hbm
        FROM_STRING_VALUE(object, hbmProfiling);
        FROM_STRING_VALUE(object, hbm_profiling_events);
        FROM_INT_VALUE(object, hbmInterval, DEFAULT_PROFILING_INTERVAL_100MS);
        // for debug purpose
        FROM_STRING_VALUE(object, stream_enabled);
        FROM_INT_VALUE(object, profiling_period, DEFAULT_PROFILING_INTERVAL_5MS);
        // host
        FROM_STRING_VALUE(object, host_sys);
        FROM_STRING_VALUE(object, host_sys_usage);
        FROM_INT_VALUE(object, host_sys_pid, HOST_PID_DEFAULT);
        FROM_STRING_VALUE(object, host_disk_profiling);
        FROM_STRING_VALUE(object, host_osrt_profiling);
        FROM_INT_VALUE(object, host_disk_freq, DEFAULT_PROFILING_INTERVAL_10MS);
        FROM_INT_VALUE(object, host_cpu_profiling_sampling_interval, DEFAULT_PROFILING_INTERVAL_20MS);
        FROM_INT_VALUE(object, host_mem_profiling_sampling_interval, DEFAULT_PROFILING_INTERVAL_20MS);
        FROM_STRING_VALUE(object, host_one_pid_mem_profiling);
        FROM_STRING_VALUE(object, host_all_pid_mem_profiling);
        FROM_STRING_VALUE(object, host_network_profiling);
        FROM_STRING_VALUE(object, host_sys_cpu_profiling);
        FROM_STRING_VALUE(object, host_sys_mem_profiling);
        GetUint64Value(object, MSG_STR(dataTypeConfig), dataTypeConfig, 0);
    }

    void FromObject(const nlohmann::json &object)
    {
        FromObjectPartOne(object);
        FromObjectPartTwo(object);
        FromObjectPartThree(object);
    }
};
}  // namespace message
}  // namespace dvvp
}  // namespace analysis
#endif // ANALYSIS_DVVP_MESSAGE_PROFILE_PARAMS_H
