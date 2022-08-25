/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: parse input params
 * Author: zyw
 * Create: 2020-12-10
 */

#ifndef INPUT_PARSER_H
#define INPUT_PARSER_H
#include <string>
#include <utils/utils.h>
#include <cstdlib>
#include "proto/profiler.pb.h"
#include "message/prof_params.h"
#include "param_validation.h"
#include "mmpa_api.h"
#include "running_mode.h"


namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using mmStructOption = Collector::Dvvp::Mmpa::mmStructOption;

const int THOUSAND = 1000; // 1000 : 1k

using MsprofStringBuffer = char *;
using MsprofString = const char *;
using MsprofStrBufAddrT = char **;

enum MsprofArgsType {
    ARGS_HELP = 0,
    // cmd
    ARGS_OUTPUT,
    ARGS_STORAGE_LIMIT,
    ARGS_APPLICATION,
    ARGS_ENVIRONMENT,
    ARGS_AIC_MODE,
    ARGS_AIC_METRICE,
    ARGS_AIV_MODE,
    ARGS_AIV_METRICS,
    ARGS_SYS_DEVICES,
    ARGS_LLC_PROFILING,
    ARGS_PYTHON_PATH,
    ARGS_SUMMARY_FORMAT,
    // switch
    ARGS_ASCENDCL,
    ARGS_AI_CORE,
    ARGS_AIV,
    ARGS_MODEL_EXECUTION,
    ARGS_RUNTIME_API,
    ARGS_TASK_TIME,
    ARGS_AICPU,
    ARGS_MSPROFTX,
    ARGS_CPU_PROFILING,
    ARGS_SYS_PROFILING,
    ARGS_PID_PROFILING,
    ARGS_HARDWARE_MEM,
    ARGS_IO_PROFILING,
    ARGS_INTERCONNECTION_PROFILING,
    ARGS_DVPP_PROFILING,
    ARGS_POWER,
    ARGS_HCCL,
    ARGS_BIU,
    ARGS_L2_PROFILING,
    ARGS_PARSE,
    ARGS_QUERY,
    ARGS_EXPORT,
    // number
    ARGS_AIC_FREQ, // 10 10-1000
    ARGS_AIV_FREQ, // 10 10-1000
    ARGS_BIU_FREQ, // 1000 300-30000
    ARGS_SYS_PERIOD, // >0
    ARGS_SYS_SAMPLING_FREQ, // 10 1-10
    ARGS_PID_SAMPLING_FREQ, // 10 1-10
    ARGS_HARDWARE_MEM_SAMPLING_FREQ, // 50 1-1000
    ARGS_IO_SAMPLING_FREQ, // 100 1-100
    ARGS_DVPP_FREQ, // 50 1-100
    ARGS_CPU_SAMPLING_FREQ, // 50 1-50
    ARGS_INTERCONNECTION_FREQ, // 50 1-50
    ARGS_EXPORT_ITERATION_ID,
    ARGS_EXPORT_MODEL_ID,
    // host
    ARGS_HOST_SYS,
    ARGS_HOST_SYS_PID,
    ARGS_HOST_USAGE,
    // end
    NR_ARGS
};

struct MsprofCmdInfo {
    char *args[NR_ARGS];
};

const mmStructOption longOptions[] = {
    // cmd
    {"help", mm_no_argument, nullptr, ARGS_HELP},
    {"output", mm_optional_argument, nullptr, ARGS_OUTPUT},
    {"storage-limit", mm_optional_argument, nullptr, ARGS_STORAGE_LIMIT},
    {"application", mm_optional_argument, nullptr, ARGS_APPLICATION},
    {"environment", mm_optional_argument, nullptr, ARGS_ENVIRONMENT},
    {"aic-mode", mm_optional_argument, nullptr, ARGS_AIC_MODE},
    {"aic-metrics", mm_optional_argument, nullptr, ARGS_AIC_METRICE},
    {"aiv-mode", mm_optional_argument, nullptr, ARGS_AIV_MODE},
    {"aiv-metrics", mm_optional_argument, nullptr, ARGS_AIV_METRICS},
    {"sys-devices", mm_optional_argument, nullptr, ARGS_SYS_DEVICES},
    {"llc-profiling", mm_optional_argument, nullptr, ARGS_LLC_PROFILING},
    {"python-path", mm_optional_argument, nullptr, ARGS_PYTHON_PATH},
    {"summary-format", mm_optional_argument, nullptr, ARGS_SUMMARY_FORMAT},
    // switch
    {"ascendcl", mm_optional_argument, nullptr, ARGS_ASCENDCL},             // the default value is on
    {"ai-core", mm_optional_argument, nullptr, ARGS_AI_CORE},
    {"ai-vector-core", mm_optional_argument, nullptr, ARGS_AIV},
    {"model-execution", mm_optional_argument, nullptr, ARGS_MODEL_EXECUTION}, // the default value is off
    {"runtime-api", mm_optional_argument, nullptr, ARGS_RUNTIME_API},  // the default value is off
    {"task-time", mm_optional_argument, nullptr, ARGS_TASK_TIME},         // the default value is on
    {"aicpu", mm_optional_argument, nullptr, ARGS_AICPU},
    {"msproftx", mm_optional_argument, nullptr, ARGS_MSPROFTX},
    {"sys-cpu-profiling", mm_optional_argument, nullptr, ARGS_CPU_PROFILING},
    {"sys-profiling", mm_optional_argument, nullptr, ARGS_SYS_PROFILING},
    {"sys-pid-profiling", mm_optional_argument, nullptr, ARGS_PID_PROFILING},
    {"sys-hardware-mem", mm_optional_argument, nullptr, ARGS_HARDWARE_MEM},
    {"sys-io-profiling", mm_optional_argument, nullptr, ARGS_IO_PROFILING},
    {"sys-interconnection-profiling", mm_optional_argument, nullptr, ARGS_INTERCONNECTION_PROFILING},
    {"dvpp-profiling", mm_optional_argument, nullptr, ARGS_DVPP_PROFILING},
    {"power", mm_optional_argument, nullptr, ARGS_POWER},
    {"hccl", mm_optional_argument, nullptr, ARGS_HCCL},  // the default value is off
    {"biu", mm_optional_argument, nullptr, ARGS_BIU},
    {"l2", mm_optional_argument, nullptr, ARGS_L2_PROFILING},
    {"parse", mm_optional_argument, nullptr, ARGS_PARSE},
    {"query", mm_optional_argument, nullptr, ARGS_QUERY},
    {"export", mm_optional_argument, nullptr, ARGS_EXPORT},
    // number
    {"aic-freq", mm_optional_argument, nullptr, ARGS_AIC_FREQ},
    {"aiv-freq", mm_optional_argument, nullptr, ARGS_AIV_FREQ},
    {"biu-freq", mm_optional_argument, nullptr, ARGS_BIU_FREQ},
    {"sys-period", mm_optional_argument, nullptr, ARGS_SYS_PERIOD},
    {"sys-sampling-freq", mm_optional_argument, nullptr, ARGS_SYS_SAMPLING_FREQ},
    {"sys-pid-sampling-freq", mm_optional_argument, nullptr, ARGS_PID_SAMPLING_FREQ},
    {"sys-hardware-mem-freq", mm_optional_argument, nullptr, ARGS_HARDWARE_MEM_SAMPLING_FREQ},
    {"sys-io-sampling-freq", mm_optional_argument, nullptr, ARGS_IO_SAMPLING_FREQ},
    {"dvpp-freq", mm_optional_argument, nullptr, ARGS_DVPP_FREQ},
    {"sys-cpu-freq", mm_optional_argument, nullptr, ARGS_CPU_SAMPLING_FREQ},
    {"sys-interconnection-freq", mm_optional_argument, nullptr, ARGS_INTERCONNECTION_FREQ},
    {"iteration-id", mm_optional_argument, nullptr, ARGS_EXPORT_ITERATION_ID},
    {"model-id", mm_optional_argument, nullptr, ARGS_EXPORT_MODEL_ID},
    // host
    {"host-sys", mm_optional_argument, nullptr, ARGS_HOST_SYS},
    {"host-sys-pid", mm_optional_argument, nullptr, ARGS_HOST_SYS_PID},
    {"host-sys-usage", mm_optional_argument, nullptr, ARGS_HOST_USAGE},
    // end
    {nullptr, mm_no_argument, nullptr, ARGS_HELP}
};

class InputParser {
public:
    InputParser();
    virtual ~InputParser();

    void MsprofCmdUsage(const std::string msg);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> MsprofGetOpts(int argc, MsprofString argv[]);
    bool HasHelpParamOnly();
private:
    int CheckPythonPathValid(const struct MsprofCmdInfo &cmdInfo) const;
    int CheckOutputValid(const struct MsprofCmdInfo &cmdInfo);
    int CheckStorageLimitValid(const struct MsprofCmdInfo &cmdInfo) const;
    int GetAppParam(const std::string &appParams);
    int CheckAppValid(const struct MsprofCmdInfo &cmdInfo);
    int CheckEnvironmentValid(const struct MsprofCmdInfo &cmdInfo);
    int CheckSampleModeValid(const struct MsprofCmdInfo &cmdInfo, int opt) const;
    int CheckArgOnOff(const struct MsprofCmdInfo &cmdInfo, int opt);
    int CheckArgRange(const struct MsprofCmdInfo &cmdInfo, int opt, int min, int max);
    int CheckAiCoreMetricsValid(const struct MsprofCmdInfo &cmdInfo, int opt) const;
    int CheckArgsIsNumber(const struct MsprofCmdInfo &cmdInfo, int opt) const;
    int CheckExportSummaryFormat(const struct MsprofCmdInfo &cmdInfo) const;
    int CheckLlcProfilingVaild(const struct MsprofCmdInfo &cmdInfo);
    int CheckSysPeriodVaild(const struct MsprofCmdInfo &cmdInfo);
    int CheckSysDevicesVaild(const struct MsprofCmdInfo &cmdInfo);
    int CheckHostSysValid(const struct MsprofCmdInfo &cmdInfo);
    int CheckHostSysPidValid(const struct MsprofCmdInfo &cmdInfo);
    int MsprofCmdCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt);
    int MsprofFreqCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt);
    int MsprofHostCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt);
    void MsprofFreqUpdateParams(const struct MsprofCmdInfo &cmdInfo, int opt);
    int MsprofSwitchCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt);
    void ParamsSwitchValid(const struct MsprofCmdInfo &cmdInfo, int opt);
    void ParamsSwitchValid2(const struct MsprofCmdInfo &cmdInfo, int opt);
    void ParamsSwitchValid3(const struct MsprofCmdInfo &cmdInfo, int opt);
    void ParamsFreqValid(const struct MsprofCmdInfo &cmdInfo, const int freq, int opt);
    int CheckLlcProfilingIsValid(const std::string &llcProfiling);
    int PreCheckApp(const std::string &appDir, const std::string &appName);
    int ParamsCheck() const;
    int HostAndDevParamsCheck();
    int ProcessOptions(int opt, struct MsprofCmdInfo &cmdInfo);
    void SetTaskTimeSwitch(const std::string timeSwitch);
    int CheckHostSysToolsIsExist(const std::string toolName);
    void SetHostSysParam(const std::string hostSysParam);
    int CheckHostSysCmdOutIsExist(const std::string tmpDir, const std::string toolName, const mmProcess tmpProcess);
    int CheckHostOutString(const std::string tmpStr, const std::string toolName);
    int UninitCheckHostSysCmd(const mmProcess checkProcess);

private:
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params_;
};

class Args {
public:
    Args(const std::string &name, const std::string &detail);
    Args(const std::string &name, const std::string &detail, const std::string &defaultValue);
    Args(const std::string &name, const std::string &detail, const std::string &defaultValue, int optional);
    virtual ~Args();
public:
    void PrintHelp();
    void SetDetail(const std::string &detail);

private:
    std::string name_;
    std::string alias_;
    std::string defaultValue_;
    std::string detail_;
    int optional_;
};

class ArgsManager : public analysis::dvvp::common::singleton::Singleton<ArgsManager> {
public:
    ArgsManager();
    virtual ~ArgsManager();
public:
    void AddArgs(const Args &args);
    void PrintHelp();

private:
    void AddHardWareMemArgs();
    void AddCpuArgs();
    void AddSysArgs();
    void AddIoArgs();
    void AddInterArgs();
    void AddDvvpArgs();
    void AddL2Args();
    void AddAivArgs();
    void AddAicpuArgs();
    void AddHostArgs();
    void AddStarsArgs();
    void AddAnalysisArgs();
private:
    std::vector<Args> argsList_;
};
}
}
}
#endif
