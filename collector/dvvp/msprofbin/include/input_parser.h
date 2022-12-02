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
#include "config/config_manager.h"


namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::Common::Config;
using MmStructOption = Collector::Dvvp::Mmpa::MmStructOption;


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
    ARGS_L2_SAMPLE_FREQ, // 100 1-100
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
    ARGS_HOST_SYS_USAGE,
    ARGS_HOST_SYS_USAGE_FREQ,
    // end
    NR_ARGS
};

struct MsprofCmdInfo {
    char *args[NR_ARGS];
};

const MmStructOption longOptions[] = {
    // cmd
    {"help", MM_NO_ARGUMENT, nullptr, ARGS_HELP},
    {"output", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_OUTPUT},
    {"storage-limit", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_STORAGE_LIMIT},
    {"application", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_APPLICATION},
    {"environment", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_ENVIRONMENT},
    {"aic-mode", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AIC_MODE},
    {"aic-metrics", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AIC_METRICE},
    {"aiv-mode", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AIV_MODE},
    {"aiv-metrics", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AIV_METRICS},
    {"sys-devices", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_SYS_DEVICES},
    {"llc-profiling", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_LLC_PROFILING},
    {"python-path", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_PYTHON_PATH},
    {"summary-format", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_SUMMARY_FORMAT},
    // switch
    {"ascendcl", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_ASCENDCL},             // the default value is on
    {"ai-core", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AI_CORE},
    {"ai-vector-core", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AIV},
    {"model-execution", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_MODEL_EXECUTION}, // the default value is off
    {"runtime-api", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_RUNTIME_API},  // the default value is off
    {"task-time", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_TASK_TIME},         // the default value is on
    {"aicpu", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AICPU},
    {"msproftx", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_MSPROFTX},
    {"sys-cpu-profiling", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_CPU_PROFILING},
    {"sys-profiling", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_SYS_PROFILING},
    {"sys-pid-profiling", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_PID_PROFILING},
    {"sys-hardware-mem", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_HARDWARE_MEM},
    {"sys-io-profiling", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_IO_PROFILING},
    {"sys-interconnection-profiling", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_INTERCONNECTION_PROFILING},
    {"dvpp-profiling", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_DVPP_PROFILING},
    {"power", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_POWER},
    {"hccl", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_HCCL},  // the default value is off
    {"biu", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_BIU},
    {"l2", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_L2_PROFILING},
    {"parse", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_PARSE},
    {"query", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_QUERY},
    {"export", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_EXPORT},
    // number
    {"aic-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AIC_FREQ},
    {"aiv-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_AIV_FREQ},
    {"l2-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_L2_SAMPLE_FREQ},
    {"biu-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_BIU_FREQ},
    {"sys-period", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_SYS_PERIOD},
    {"sys-sampling-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_SYS_SAMPLING_FREQ},
    {"sys-pid-sampling-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_PID_SAMPLING_FREQ},
    {"sys-hardware-mem-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_HARDWARE_MEM_SAMPLING_FREQ},
    {"sys-io-sampling-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_IO_SAMPLING_FREQ},
    {"dvpp-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_DVPP_FREQ},
    {"sys-cpu-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_CPU_SAMPLING_FREQ},
    {"sys-interconnection-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_INTERCONNECTION_FREQ},
    {"iteration-id", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_EXPORT_ITERATION_ID},
    {"model-id", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_EXPORT_MODEL_ID},
    // host
    {"host-sys", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_HOST_SYS},
    {"host-sys-pid", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_HOST_SYS_PID},
    {"host-sys-usage", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_HOST_SYS_USAGE},
    {"host-sys-usage-freq", MM_OPTIONAL_ARGUMENT, nullptr, ARGS_HOST_SYS_USAGE_FREQ},
    // end
    {nullptr, MM_NO_ARGUMENT, nullptr, ARGS_HELP}
};

class InputParser {
public:
    InputParser();
    virtual ~InputParser();

    void MsprofCmdUsage(const std::string msg);
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> MsprofGetOpts(int argc, MsprofString argv[]);
    bool HasHelpParamOnly();

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
    void PrintHelp();

private:
    void Init();
    void AddHardWareMemArgs();
    void AddCpuArgs();
    void AddSysArgs();
    void AddIoArgs();
    void AddInterArgs();
    void AddDvvpArgs();
    void AddL2Args();
    void AddAivArgs();
    void AddBiuArgs();
    void AddAicpuArgs();
    void AddHostArgs();
    void AddStarsArgs();
    void AddAnalysisArgs();
private:
    bool driverOnline_;
    PlatformType platform_;
    std::vector<Args> argsList_;
};
}
}
}
#endif
