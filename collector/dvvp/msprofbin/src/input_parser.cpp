/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: parse input params
 * Author: zyw
 * Create: 2020-12-10
 */
#include "input_parser.h"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include "cmd_log.h"
#include "errno/error_code.h"
#include "param_validation.h"
#include "utils/utils.h"
#include "config/config_manager.h"
#include "ai_drv_dev_api.h"
#include "platform/platform.h"
#include "config/config.h"
#include "msprof_dlog.h"
#include "mmpa_api.h"
#include "params_adapter_impl.h"

namespace Analysis {
namespace Dvvp {
namespace Msprof {
using namespace analysis::dvvp::driver;
using namespace analysis::dvvp::common::validation;
using namespace analysis::dvvp::common::utils;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace analysis::dvvp::common::config;
using namespace Collector::Dvvp::Msprofbin;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;
using namespace Collector::Dvvp::ParamsAdapter;

const int MSPROF_DAEMON_ERROR       = -1;
const int MSPROF_DAEMON_OK          = 0;
const int FILE_FIND_REPLAY          = 100;
const std::string TASK_BASED        = "task-based";
const std::string SAMPLE_BASED      = "sample-based";
const std::string ALL               = "all";
const std::string ON                = "on";
const std::string OFF               = "off";
const std::string LLC_CAPACITY      = "capacity";
const std::string LLC_BANDWIDTH     = "bandwidth";
const std::string LLC_READ          = "read";
const std::string LLC_WRITE         = "write";
const std::string TOOL_NAME_PERF    = "perf";
const std::string TOOL_NAME_LTRACE  = "ltrace";
const std::string TOOL_NAME_IOTOP   = "iotop";
const std::string CSV_FORMAT        = "csv";
const std::string JSON_FORMAT       = "json";


InputParser::InputParser()
{
    MSVP_MAKE_SHARED0_VOID(params_, analysis::dvvp::message::ProfileParams);
}

InputParser::~InputParser()
{}

void InputParser::MsprofCmdUsage(const std::string msg)
{
    if (!msg.empty()) {
        std::cout << "err: " << const_cast<CHAR_PTR>(msg.c_str()) << std::endl;
    }
    ArgsManager::instance()->PrintHelp();
}

SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> InputParser::MsprofGetOpts(int argc, CONST_CHAR_PTR argv[])
{
    const static int inputMaxLen = 512; // 512 : max length
    if (argc > inputMaxLen || argv == nullptr || strlen(*argv) > inputMaxLen) {
        CmdLog::instance()->CmdErrorLog("input data is invalid,"
            "please input argc less than 512 and argv is not null and the len of argv less than 512");
        return nullptr;
    }

    int opt = 0;
    int optionIndex = 0;
    MsprofString optString  = "";
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    std::string argvStr = "";
    while ((opt = MmGetOptLong(argc, const_cast<MsprofStrBufAddrT>(argv),
        optString, longOptions, &optionIndex)) != MSPROF_DAEMON_ERROR) {
        if (opt < ARGS_HELP || opt >= NR_ARGS) {
            MsprofCmdUsage("");
            return nullptr;
        }
        cmdInfo.args[opt] = MmGetOptArg();
        argvStr = std::string(argv[MmGetOptInd() - 1]);
        argvMap.insert({opt, std::pair<MsprofCmdInfo, std::string>(cmdInfo, argvStr)});
        params_->usedParams.insert(opt);
    }
    auto paramAdapter = MsprofParamAdapter();
    int ret = paramAdapter.GetParamFromInputCfg(argvMap, params_);
    if (ret != PROFILING_SUCCESS) {
        MsprofCmdUsage("get params from input config failed.");
        return nullptr;
    }
    return params_;
}

bool InputParser::HasHelpParamOnly()
{
    bool ret = false;
    if (params_ == nullptr) {
        return ret;
    }
    if (params_->usedParams.size() == 1 &&
        params_->usedParams.count(ARGS_HELP)) {
        ret = true;
    }
    return ret;
}

Args::Args(const std::string &name, const std::string &detail)
    : name_(name), detail_(detail), optional_(mm_optional_argument)
{
}

Args::Args(const std::string &name, const std::string &detail, const std::string &defaultValue)
    : name_(name), defaultValue_(defaultValue), detail_(detail), optional_(mm_optional_argument)
{
}

Args::Args(const std::string &name, const std::string &detail, const std::string &defaultValue, int optional)
    : name_(name), defaultValue_(defaultValue), detail_(detail), optional_(optional)
{
}

Args::~Args()
{
}

void Args::PrintHelp()
{
    std::string ifOptional = (optional_ == mm_optional_argument) ? "<Optional>" : "<Mandatory>";
    std::cout << std::right << std::setw(8) << "--"; // 8 space
    std::cout << std::left << std::setw(32) << name_  << ifOptional; // 32 space for option
    std::cout << " " << detail_ << std::endl << std::flush;
}

void Args::SetDetail(const std::string &detail)
{
    detail_ = detail;
}

ArgsManager::ArgsManager()
{
    argsList_ = {
    {"output", "Specify the directory that is used for storing data results."},
    {"storage-limit", "Specify the output directory volume. range 200MB ~ 4294967296MB"},
    {"application", "Specify application path, considering the risk of privilege escalation, please pay attention to\n"
        "\t\t\t\t\t\t   the group of the application and confirm whether it is the same as the user currently"},
    {"ascendcl", "Show acl profiling data, the default value is on.", ON},
    {"model-execution", "Show ge model execution profiling data, the default value is off.", OFF},
    {"runtime-api", "Show runtime api profiling data, the default value is off.", OFF},
    {"task-time", "Show task profiling data, the default value is on.", ON},
    {"ai-core", "Turn on / off the ai core profiling, the default value is on when collecting app Profiling.", ON},
    {"aic-mode", "Set the aic profiling mode to task-based or sample-based.\n"
                  "\t\t\t\t\t\t   In task-based mode, profiling data will be collected by tasks.\n"
                  "\t\t\t\t\t\t   In sample-based mode, profiling data will be collected in a specific interval.\n"
                  "\t\t\t\t\t\t   The default value is task-based.", TASK_BASED},
    {"aic-freq", "The aic sampling frequency in hertz, "
                "the default value is 100 Hz, the range is 1 to 100 Hz.", "100"},
    {"aic-metrics", "The aic metrics groups, "
        "include ArithmeticUtilization, PipeUtilization, "
        "Memory, MemoryL0, ResourceConflictRatio, MemoryUB.\n"
        "\t\t\t\t\t\t   the default value is PipeUtilization.", "PipeUtilization"},
    {"environment", "User app custom environment variable configuration."},
    {"sys-period", "Set total sampling period of system profiling in seconds."},
    {"sys-devices", "Specify the profiling scope by device ID when collect sys profiling."
                     "The value is all or ID list (split with ',')."},
    {"hccl", "Show hccl profiling data, the default value is off.", OFF},
    {"biu", "Show biu profiling data, the default value is off.", OFF},
    {"biu-freq", "The biu sampling period in clock-cycle, "
                "the default value is 1000 cycle, the range is 300 to 30000 cycle.", "1000"},
    {"msproftx", "Show msproftx data, the default value is off.", OFF}
    };
    AddAnalysisArgs();
    AddAicpuArgs();
    AddAivArgs();
    AddHardWareMemArgs();
    AddCpuArgs();
    AddSysArgs();
    AddIoArgs();
    AddInterArgs();
    AddDvvpArgs();
    AddL2Args();
    AddHostArgs();
    AddStarsArgs();
    Args help = {"help", "help message."};
    argsList_.push_back(help);
}

ArgsManager::~ArgsManager()
{
    argsList_.clear();
}

void ArgsManager::AddAnalysisArgs()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MDC_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE) {
        return;
    }
    std::vector<Args> argsList;
    argsList = {
    {"python-path", "Specify the python interpreter path that is used for analysis, please ensure the python version"
                " is 3.7.5 or later."},
    {"parse", "Switch for using msprof to parse collecting data, the default value is off.", OFF},
    {"query", "Switch for using msprof to query collecting data, the default value is off.", OFF},
    {"export", "Switch for using msprof to export collecting data, the default value is off.", OFF},
    {"iteration-id", "The export iteration id, only uesd when argument export is on, the default value is 1", "1"},
    {"model-id", "The export model id, only uesd when argument export is on, "
        "msprof will export minium accessible model by default.",
        "-1"},
    {"summary-format", "The export summary file format, only uesd when argument export is on, "
        "include csv, json, the default value is csv.", "csv"}
    };
    argsList_.insert(argsList_.end(), argsList.begin(), argsList.end());
}

void ArgsManager::AddStarsArgs()
{
    if (ConfigManager::instance()->GetPlatformType() != PlatformType::CHIP_V4_1_0) {
        return;
    }

    Args lowPowerArgs = {"power", "Show low power profiling data, the default value is off."};
    argsList_.push_back(lowPowerArgs);
}

void ArgsManager::AddAicpuArgs()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MDC_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE) {
        return;
    }
    Args aicpu = {"aicpu", "Show aicpu profiling data, the default value is off.", OFF};
    argsList_.push_back(aicpu);
}

void ArgsManager::AddAivArgs()
{
    if (ConfigManager::instance()->GetPlatformType() != PlatformType::MDC_TYPE &&
        ConfigManager::instance()->GetPlatformType() != PlatformType::CHIP_V4_1_0) {
        return;
    }
    Args aiv = {"ai-vector-core", "Turn on / off the ai vector core profiling, the default value is on.", ON};
    Args aivMode = {"aiv-mode", "Set the aiv profiling mode to task-based or sample-based.\n"
        "\t\t\t\t\t\t   In task-based mode, profiling data will be collected by tasks.\n"
        "\t\t\t\t\t\t   In sample-based mode, profiling data will be collected in a specific interval.\n"
        "\t\t\t\t\t\t   The default value is task-based.",
        TASK_BASED};
    Args aivFreq = {"aiv-freq", "The aiv sampling frequency in hertz, "
        "the default value is 100 Hz, the range is 1 to 100 Hz.",
        "100"};
    Args aivMetrics = {"aiv-metrics", "The aiv metrics groups, "
        "include ArithmeticUtilization, PipeUtilization, "
        "Memory, MemoryL0, ResourceConflictRatio, MemoryUB.\n"
        "\t\t\t\t\t\t   the default value is PipeUtilization.",
        "PipeUtilization"};
    argsList_.push_back(aiv);
    argsList_.push_back(aivMode);
    argsList_.push_back(aivFreq);
    argsList_.push_back(aivMetrics);
}

void ArgsManager::AddHardWareMemArgs()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE) {
        return;
    }
    auto hardwareMem = Args("sys-hardware-mem", "", OFF);
    auto hardwareMemFreq = Args("sys-hardware-mem-freq", "", "50");
    auto llcProfiling = Args("llc-profiling", "", "capacity");
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        hardwareMem.SetDetail("LLC, DDR acquisition switch, optional on / off, the default value is off.");
        hardwareMemFreq.SetDetail("LLC, DDR acquisition frequency, range 1 ~ 1000, "
                               "the default value is 50, unit Hz.");
        llcProfiling.SetDetail("The llc profiling groups, include capacity, bandwidth. the default value is capacity.");
    } else {
        hardwareMem.SetDetail("LLC, DDR, HBM acquisition switch.");
        hardwareMemFreq.SetDetail("LLC, DDR,HBM acquisition frequency, range 1 ~ 1000, "
                               "the default value is 50, unit Hz.");
        llcProfiling.SetDetail("The llc profiling groups, include read, write. the default value is read.");
    }
    argsList_.push_back(hardwareMem);
    argsList_.push_back(hardwareMemFreq);
    argsList_.push_back(llcProfiling);
}

void ArgsManager::AddCpuArgs()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE) {
        return;
    }
    Args cpu = Args("sys-cpu-profiling", "The CPU acquisition switch, optional on / off,"
        "the default value is off.",
        OFF);
    Args cpuFreq =  {"sys-cpu-freq", "The cpu sampling frequency in hertz. "
        "the default value is 50 Hz, the range is 1 to 50 Hz.",
        "50"};
    argsList_.push_back(cpu);
    argsList_.push_back(cpuFreq);
}

void ArgsManager::AddSysArgs()
{
    Args sysProfiling = {"sys-profiling", "The System CPU usage and system memory acquisition switch,"
        "the default value is off.",
        OFF};
    Args sysFreq = {"sys-sampling-freq", "The sys sampling frequency in hertz. "
        "the default value is 10 Hz, the range is 1 to 10 Hz.",
        "10"};
    Args pidProfiling = {"sys-pid-profiling",
        "The CPU usage of the process and the memory acquisition switch of the process,"
        "the default value is off.",
        OFF};
    Args pidFreq = {"sys-pid-sampling-freq", "The pid sampling frequency in hertz. "
        "the default value is 10 Hz, the range is 1 to 10 Hz.",
        "10"};
    argsList_.push_back(sysProfiling);
    argsList_.push_back(sysFreq);
    argsList_.push_back(pidProfiling);
    argsList_.push_back(pidFreq);
}

void ArgsManager::AddIoArgs()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::DC_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::MDC_TYPE) {
        return;
    }
    Args ioArgs = {"sys-io-profiling", "NIC acquisition switch, the default value is off.", OFF};
    Args ioFreqArgs = {"sys-io-sampling-freq", "NIC acquisition frequency, range 1 ~ 100, "
        "the default value is 100, unit Hz.",
        "100"};

    if (ConfigManager::instance()->GetPlatformType() == PlatformType::CLOUD_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::CHIP_V4_1_0) {
        ioArgs.SetDetail("NIC, ROCE acquisition switch, the default value is off.");
        ioFreqArgs.SetDetail("NIC, ROCE acquisition frequency, range 1 ~ 100, "
                               "the default value is 100, unit Hz.");
    }
    argsList_.push_back(ioArgs);
    argsList_.push_back(ioFreqArgs);
}

void ArgsManager::AddInterArgs()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::MDC_TYPE) {
        return;
    }
    Args interArgs = {"sys-interconnection-profiling",
        "PCIE, HCCS acquisition switch, the default value is off.",
        OFF};
    Args interFreq = {"sys-interconnection-freq", "PCIE, HCCS acquisition frequency, range 1 ~ 50, "
        "the default value is 50, unit Hz.",
        "50"};
    if (ConfigManager::instance()->GetPlatformType() != PlatformType::CLOUD_TYPE &&
        ConfigManager::instance()->GetPlatformType() != PlatformType::CHIP_V4_1_0) {
        interArgs = {"sys-interconnection-profiling",
            "PCIE acquisition switch, the default value is off.",
            OFF};
        interFreq = {"sys-interconnection-freq", "PCIE acquisition frequency, range 1 ~ 50, "
            "the default value is 50, unit Hz.", "50"};
    }
    argsList_.push_back(interArgs);
    argsList_.push_back(interFreq);
}

void ArgsManager::AddDvvpArgs()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE) {
        return;
    }
    Args dvpp = {"dvpp-profiling",
        "DVPP acquisition switch, the default value is off.",
        OFF};
    Args dvppFreq = {"dvpp-freq", "DVPP acquisition frequency, range 1 ~ 100, "
        "the default value is 50, unit Hz.",
        "50"};
    argsList_.push_back(dvpp);
    argsList_.push_back(dvppFreq);
}

void ArgsManager::AddL2Args()
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::LHISI_TYPE ||
        ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        return;
    }
    Args l2 = {"l2", "L2 Cache acquisition switch. the default value is off.", OFF};
    argsList_.push_back(l2);
}

void ArgsManager::PrintHelp()
{
    std::cout << std::endl << "Usage:" << std::endl;
    std::cout << "      ./msprof [--options]" << std::endl << std::endl;
    std::cout << "Options:" << std::endl;
    for (auto args : argsList_) {
        args.PrintHelp();
    }
}

void ArgsManager::AddHostArgs()
{
    if (Platform::instance()->RunSocSide()) {
        return;
    }
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    return;
#endif
    Args hostSys = {"host-sys", "The host-sys data type, include cpu, mem, disk, network, osrt",
        HOST_SYS_CPU};
    Args hostSysPid = {"host-sys-pid", "Set the PID of the app process for "
        "which you want to collect performance data."};
    argsList_.push_back(hostSys);
    argsList_.push_back(hostSysPid);
}
}
}
}
