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
#include "msprof_dlog.h"
#include "mmpa_api.h"
#include "params_adapter_msprof.h"

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
        CmdLog::instance()->CmdErrorLog("%s", msg.c_str());
    }
    ArgsManager::instance()->PrintHelp();
}

SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> InputParser::MsprofGetOpts(int argc, CONST_CHAR_PTR argv[])
{
    if (params_ == nullptr) {
        MSPROF_LOGE("params_ is null in InputParser.");
        return nullptr;
    }
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
        if (opt == ARGS_HELP) {
            params_->usedParams.clear();
            params_->usedParams.insert(opt);
            MsprofCmdUsage("");
            return params_;
        }
        cmdInfo.args[opt] = MmGetOptArg();
        argvStr = std::string(argv[MmGetOptInd() - 1]);
        if (cmdInfo.args[opt] == nullptr) {
            CmdLog::instance()->CmdErrorLog("Argument %s empty value.", argvStr.c_str());
            MsprofCmdUsage("");
            return nullptr;
        }
        if (argvMap.find(opt) == argvMap.end()) {
            argvMap.insert({opt, std::pair<MsprofCmdInfo, std::string>(cmdInfo, argvStr)});
            params_->usedParams.insert(opt);
        } else {
            argvMap[opt] = std::pair<MsprofCmdInfo, std::string>(cmdInfo, argvStr);
        }
    }
    auto paramAdapter = ParamsAdapterMsprof();
    if (paramAdapter.GetParamFromInputCfg(argvMap, params_) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[MsprofGetOpts]get param from InputCfg failed.");
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
    : name_(name), detail_(detail), optional_(MM_OPTIONAL_ARGUMENT)
{
}

Args::Args(const std::string &name, const std::string &detail, const std::string &defaultValue)
    : name_(name), defaultValue_(defaultValue), detail_(detail), optional_(MM_OPTIONAL_ARGUMENT)
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
    std::string ifOptional = (optional_ == MM_OPTIONAL_ARGUMENT) ? "<Optional>" : "<Mandatory>";
    std::cout << std::right << std::setw(8) << "--"; // 8 space
    std::cout << std::left << std::setw(32) << name_  << ifOptional; // 32 space for option
    std::cout << " " << detail_ << std::endl << std::flush;
}

void Args::SetDetail(const std::string &detail)
{
    detail_ = detail;
}

ArgsManager::ArgsManager() : driverOnline_(false)
{
}

ArgsManager::~ArgsManager()
{
    argsList_.clear();
}

void ArgsManager::Init()
{
    driverOnline_ = Platform::instance()->DriverAvailable();
    if (driverOnline_) {
        platform_ = ConfigManager::instance()->GetPlatformType();
    }
    argsList_ = {
        {"output", "Specify the directory that is used for storing data results.(full-platform)"},
        {"storage-limit", "Specify the output directory volume. range 200MB ~ 4294967296MB.(full-platform)"},
        {"application", "Specify application path, considering the risk of privilege escalation,\n"
            "\t\t\t\t\t\t   please pay attention to the group of the application and \n"
            "\t\t\t\t\t\t   confirm whether it is the same as the user currently.(full-platform)"},
        {"ascendcl", "Show acl profiling data, the default value is on.(full-platform)", ON},
        {"model-execution", "Show ge model execution profiling data, the default value is off.(full-platform)", OFF},
        {"runtime-api", "Show runtime api profiling data, the default value is off.(full-platform)", OFF},
        {"task-time", "Show task profiling data, the default value is on.(full-platform)", ON},
        {"ai-core", "Turn on / off the ai core profiling, the default value is on when collecting\n"
            "\t\t\t\t\t\t   app Profiling.(full-platform)", ON},
        {"aic-mode", "Set the aic profiling mode to task-based or sample-based.(full-platform)\n"
            "\t\t\t\t\t\t   In task-based mode, profiling data will be collected by tasks.\n"
            "\t\t\t\t\t\t   In sample-based mode, profiling data will be collected in a specific interval.\n"
            "\t\t\t\t\t\t   The default value is task-based.", TASK_BASED},
        {"aic-freq", "The aic sampling frequency in hertz, the default value is 100 Hz, the range is\n"
            "\t\t\t\t\t\t   1 to 100 Hz.(full-platform)", "100"},
        {"aic-metrics", "The aic metrics groups, include ArithmeticUtilization, PipeUtilization,\n"
            "\t\t\t\t\t\t   Memory, MemoryL0, ResourceConflictRatio, MemoryUB. the default value is\n"
            "\t\t\t\t\t\t   PipeUtilization.(full-platform)", "PipeUtilization"},
        {"environment", "User app custom environment variable configuration.(full-platform)"},
        {"sys-period", "Set total sampling period of system profiling in seconds.(full-platform)"},
        {"sys-devices", "Specify the profiling scope by device ID when collect sys profiling.\n"
            "\t\t\t\t\t\t   The value is all or ID list (split with ',').(full-platform)"},
        {"hccl", "Show hccl profiling data, the default value is off.(full-platform)", OFF},
        {"msproftx", "Show msproftx data, the default value is off.(full-platform)", OFF}
    };
    AddDynProfArgs();
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
    AddBiuArgs();
    AddHostArgs();
    AddStarsArgs();
    Args help = {"help", "help message.(full-platform)"};
    argsList_.push_back(help);
}

void ArgsManager::AddDynProfArgs()
{
    if (driverOnline_ && platform_ != PlatformType::CLOUD_TYPE) {
        return;
    }
    Args dynamic = {"dynamic", "Dynamic profiling switch, the default value is off.(Ascend910)", OFF};
    Args pid = {"pid", "Dynamic profiling pid of the target process.(Ascend910)", "0"};
    argsList_.push_back(dynamic);
    argsList_.push_back(pid);
}

void ArgsManager::AddAnalysisArgs()
{
    if (driverOnline_ && (platform_ == PlatformType::MDC_TYPE || platform_ == PlatformType::LHISI_TYPE)) {
        return;
    }
    std::vector<Args> argsList;
    argsList = {
        {"python-path", "Specify the python interpreter path that is used for analysis, please\n"
            "\t\t\t\t\t\t   ensure the python version is 3.7.5 or later.(full-platform)"},
        {"parse", "Switch for using msprof to parse collecting data, the default value\n"
            "\t\t\t\t\t\t   is off.(full-platform)", OFF},
        {"query", "Switch for using msprof to query collecting data, the default value\n"
            "\t\t\t\t\t\t   is off.(full-platform)", OFF},
        {"export", "Switch for using msprof to export collecting data, the default value\n"
            "\t\t\t\t\t\t   is off.(full-platform)", OFF},
        {"iteration-id", "The export iteration id, only uesd when argument export is on,\n"
            "\t\t\t\t\t\t   the default value is 1", "1"},
        {"model-id", "The export model id, only uesd when argument export is on, msprof will\n"
            "\t\t\t\t\t\t   export minium accessible model by default.(full-platform)", "-1"},
        {"summary-format", "The export summary file format, only uesd when argument export is on,\n"
            "\t\t\t\t\t\t   include csv, json, the default value is csv.(full-platform)", "csv"}
    };
    argsList_.insert(argsList_.end(), argsList.begin(), argsList.end());
}

void ArgsManager::AddStarsArgs()
{
    if (driverOnline_ && platform_ != PlatformType::CHIP_V4_1_0 && platform_ != PlatformType::CHIP_V4_2_0) {
        return;
    }

    Args lowPowerArgs = {"power", "Show low power profiling data, the default value is off.(future-platform)"};
    argsList_.push_back(lowPowerArgs);
}

void ArgsManager::AddBiuArgs()
{
    if (driverOnline_ && platform_ != PlatformType::CHIP_V4_1_0) {
        return;
    }
    Args biu = {"biu", "Show biu profiling data, the default value is off.(future-platform)", OFF};
    Args biuFreq = {"biu-freq",
                    "The biu sampling period in clock-cycle, the default value is 1000 cycle,\n"
                        "\t\t\t\t\t\t   the range is 300 to 30000 cycle.(future-platform)",
                    "1000"};
    argsList_.push_back(biu);
    argsList_.push_back(biuFreq);
}

void ArgsManager::AddAicpuArgs()
{
    if (driverOnline_ && (platform_ == PlatformType::MDC_TYPE || platform_ == PlatformType::LHISI_TYPE)) {
        return;
    }
    Args aicpu = {"aicpu", "Show aicpu profiling data, the default value is off.(full-platform)", OFF};
    argsList_.push_back(aicpu);
}

void ArgsManager::AddAivArgs()
{
    if (driverOnline_ && (platform_ != PlatformType::MDC_TYPE)) {
        return;
    }
    Args aiv = {"ai-vector-core",
                "Turn on / off the ai vector core profiling, the default value\n"
                    "\t\t\t\t\t\t   is on.(future-platform)",
                ON};
    Args aivMode = {"aiv-mode",
                    "Set the aiv profiling mode to task-based or sample-based.(future-platform)\n"
                        "\t\t\t\t\t\t   In task-based mode, profiling data will be collected by tasks.\n"
                        "\t\t\t\t\t\t   In sample-based mode, profiling data will be collected "
                        "in a specific interval.\n"
                        "\t\t\t\t\t\t   The default value is task-based.",
                    TASK_BASED};
    Args aivFreq = {"aiv-freq",
                    "The aiv sampling frequency in hertz, the default value is 100 Hz,\n"
                        "\t\t\t\t\t\t   the range is 1 to 100 Hz.(future-platform)",
                    "100"};
    Args aivMetrics = {"aiv-metrics",
                       "The aiv metrics groups, include ArithmeticUtilization, PipeUtilization,\n"
                            "\t\t\t\t\t\t   Memory, MemoryL0, ResourceConflictRatio, MemoryUB. the default value\n"
                            "\t\t\t\t\t\t   is PipeUtilization.(future-platform)",
                       "PipeUtilization"};
    argsList_.push_back(aiv);
    argsList_.push_back(aivMode);
    argsList_.push_back(aivFreq);
    argsList_.push_back(aivMetrics);
}

void ArgsManager::AddHardWareMemArgs()
{
    if (driverOnline_ && platform_ == PlatformType::LHISI_TYPE) {
        return;
    }
    auto hardwareMem = Args("sys-hardware-mem", "", OFF);
    auto hardwareMemFreq = Args("sys-hardware-mem-freq", "", "50");
    auto llcMode = Args("llc-profiling", "", "capacity");
    hardwareMem.SetDetail("LLC, DDR, HBM acquisition switch, optional on / off, the default value is off.\n"
                              "\t\t\t\t\t\t   LLC, DDR(full-platform), HBM(Ascend910)");
    hardwareMemFreq.SetDetail("LLC, DDR, HBM acquisition frequency, range 1 ~ 1000, the default value is 50 Hz.\n"
                                  "\t\t\t\t\t\t   LLC, DDR(full-platform), HBM(Ascend910)");
    if (driverOnline_ && platform_ == PlatformType::MINI_TYPE) {
        llcMode.SetDetail("The llc profiling groups. Include capacity, bandwidth. "
            "the default value is capacity.(Ascend310)");
    } else if (driverOnline_ && (platform_ == PlatformType::DC_TYPE || platform_ == PlatformType::CLOUD_TYPE)) {
        llcMode.SetDetail("The llc profiling groups. Include read, write. "
            "the default value is read.(Ascend310P, Ascend910)");
    } else {
        llcMode.SetDetail("The llc profiling groups.\n"
                              "\t\t\t\t\t\t   include capacity, bandwidth. the default value is capacity.(Ascend310)\n"
                              "\t\t\t\t\t\t   include read, write. the default value is read.(Ascend310P, Ascend910)");
    }
    
    argsList_.push_back(hardwareMem);
    argsList_.push_back(hardwareMemFreq);
    argsList_.push_back(llcMode);
}

void ArgsManager::AddCpuArgs()
{
    if (driverOnline_ && platform_ == PlatformType::LHISI_TYPE) {
        return;
    }
    Args cpu = Args("sys-cpu-profiling",
                    "The CPU acquisition switch, optional on / off, the default value\n"
                        "\t\t\t\t\t\t   is off.(full-platform)", OFF);
    Args cpuFreq = {"sys-cpu-freq",
                    "The cpu sampling frequency in hertz. the default value\n"
                        "\t\t\t\t\t\t   is 50 Hz, the range is 1 to 50 Hz.(full-platform)",
                    "50"};
    argsList_.push_back(cpu);
    argsList_.push_back(cpuFreq);
}

void ArgsManager::AddSysArgs()
{
    Args sysProfiling = {"sys-profiling",
                         "The System CPU usage and system memory acquisition switch,\n"
                             "\t\t\t\t\t\t   the default value is off.(full-platform)",
                         OFF};
    Args sysFreq = {"sys-sampling-freq",
                    "The sys sampling frequency in hertz. the default value\n"
                        "\t\t\t\t\t\t   is 10 Hz, the range is 1 to 10 Hz.(full-platform)",
                    "10"};
    Args pidProfiling = {"sys-pid-profiling",
                         "The CPU usage of the process and the memory acquisition\n"
                             "\t\t\t\t\t\t   switch of the process, the default value is off.(full-platform)",
                         OFF};
    Args pidFreq = {"sys-pid-sampling-freq",
                    "The pid sampling frequency in hertz. the default value is 10 Hz,\n"
                        "\t\t\t\t\t\t   the range is 1 to 10 Hz.(full-platform)",
                    "10"};
    argsList_.push_back(sysProfiling);
    argsList_.push_back(sysFreq);
    argsList_.push_back(pidProfiling);
    argsList_.push_back(pidFreq);
}

void ArgsManager::AddIoArgs()
{
    if (driverOnline_ && (platform_ == PlatformType::LHISI_TYPE || platform_ == PlatformType::DC_TYPE ||
        platform_ == PlatformType::MDC_TYPE)) {
        return;
    }
    Args ioArgs = {"sys-io-profiling",
                   "NIC ROCE acquisition switch, the default value is off.\n"
                        "\t\t\t\t\t\t   NIC(Ascend310, Ascend910), ROCE(Ascend910)",
                   OFF};
    Args ioFreqArgs = {"sys-io-sampling-freq",
                       "NIC ROCE acquisition frequency, range 1 ~ 100, the default value is 100 Hz.\n"
                           "\t\t\t\t\t\t   NIC(Ascend310, Ascend910), ROCE(Ascend910)",
                       "100"};
    argsList_.push_back(ioArgs);
    argsList_.push_back(ioFreqArgs);
}

void ArgsManager::AddInterArgs()
{
    if (driverOnline_ && (platform_ == PlatformType::LHISI_TYPE || platform_ == PlatformType::MINI_TYPE ||
        platform_ == PlatformType::MDC_TYPE)) {
        return;
    }
    Args interArgs = {"sys-interconnection-profiling",
                      "PCIE, HCCS acquisition switch, the default value is off.\n"
                          "\t\t\t\t\t\t   PCIE(Ascend310P, Ascend910), HCCS(Ascend910)",
                      OFF};
    Args interFreq = {"sys-interconnection-freq",
                      "PCIE, HCCS acquisition frequency, range 1 ~ 50, the default value is 50 Hz.\n"
                          "\t\t\t\t\t\t   PCIE(Ascend310P, Ascend910), HCCS(Ascend910)",
                      "50"};
    argsList_.push_back(interArgs);
    argsList_.push_back(interFreq);
}

void ArgsManager::AddDvvpArgs()
{
    if (driverOnline_ && platform_ == PlatformType::LHISI_TYPE) {
        return;
    }
    Args dvpp = {"dvpp-profiling", "DVPP acquisition switch, the default value is off.(full-platform)", OFF};
    Args dvppFreq = {"dvpp-freq",
                     "DVPP acquisition frequency, range 1 ~ 100, the default value is 50 Hz.(full-platform)",
                     "50"};
    argsList_.push_back(dvpp);
    argsList_.push_back(dvppFreq);
}

void ArgsManager::AddL2Args()
{
    if (driverOnline_ && (platform_ == PlatformType::LHISI_TYPE || platform_ == PlatformType::MINI_TYPE)) {
        return;
    }
    Args l2 = {"l2", "L2 Cache acquisition switch. the default value is off.(Ascend310P, Ascend910)", OFF};
    argsList_.push_back(l2);
    if (driverOnline_ && platform_ != PlatformType::CHIP_V4_1_0 && platform_ != PlatformType::CHIP_V4_2_0) {
        return;
    }
    Args l2freq = {"l2-freq",
                   "L2 Cache acquisition frequency, range 1 ~ 100, the default value is 100 Hz.(future-platform)",
                   "100"};
    argsList_.push_back(l2freq);
}

void ArgsManager::PrintHelp()
{
    Init();
    std::cout << std::endl << "Usage:" << std::endl;
    std::cout << "      ./msprof [--options]" << std::endl << std::endl;
    std::cout << "Options:" << std::endl;
    for (auto args : argsList_) {
        args.PrintHelp();
    }
}

void ArgsManager::AddHostArgs()
{
    if (driverOnline_ && Platform::instance()->RunSocSide()) {
        return;
    }
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    return;
#endif
    Args hostSys = {"host-sys", "The host-sys data type, include cpu, mem, disk, network, osrt.(full-platform)",
        HOST_SYS_CPU};
    Args hostSysPid = {"host-sys-pid",
                       "Set the PID of the app process for which you want to collect\n"
                           "\t\t\t\t\t\t   performance data.(full-platform)"};
    Args hostSysUsage = {"host-sys-usage",
                         "The host-sys-usage data type, include cpu, mem.(full-platform)",
                         HOST_SYS_CPU};
    Args hostSysUsageFreq = {"host-sys-usage-freq",
                             "The sampling frequency in hertz. the default value\n"
                                 "\t\t\t\t\t\t   is 50 Hz, the range is 1 to 50 Hz.(full-platform)",
                             "50"};
    argsList_.push_back(hostSys);
    argsList_.push_back(hostSysPid);
    argsList_.push_back(hostSysUsage);
    argsList_.push_back(hostSysUsageFreq);
}
}
}
}
