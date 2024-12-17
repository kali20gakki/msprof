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
#include <sstream>
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

std::string g_appStr;
std::string g_appPath;
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

bool InputParser::CheckInstrAndTaskParamBothSet(std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap)
{
    std::string comflictStr = "";
    const std::map<int, std::string> INSTR_LIST = {
        {ARGS_AI_CORE, "ai-core"},
        {ARGS_TASK_TIME, "task-time"},
        {ARGS_ASCENDCL, "ascendcl"},
        {ARGS_MODEL_EXECUTION, "model-execution"},
        {ARGS_RUNTIME_API, "runtime-api"},
        {ARGS_AICPU, "aicpu"},
        {ARGS_HCCL, "hccl"},
        {ARGS_L2_PROFILING, "l2"},
    };

    if (argvMap.count(ARGS_INSTR_PROFILING) != 0 &&
        std::string(argvMap[ARGS_INSTR_PROFILING].first.args[ARGS_INSTR_PROFILING], ON.size()) == "on") {
        for (auto &cfg : INSTR_LIST) {
            if (argvMap.count(cfg.first) != 0 && std::string(argvMap[cfg.first].first.args[cfg.first],
                ON.size()) == "on") {
                comflictStr += " ";
                comflictStr += cfg.second;
            }
        }
        if (argvMap.count(ARGS_AIC_FREQ) != 0) {
            comflictStr += " aic-freq";
        }
        if (argvMap.count(ARGS_AIC_MODE) != 0) {
            comflictStr += " aic-mode";
        }
        if (argvMap.count(ARGS_AIC_METRICE) != 0) {
            comflictStr += " aic-metrics";
        }
    }

    if (!comflictStr.empty()) {
        CmdLog::instance()->CmdErrorLog(" Profiling fails to start because instr_profiling is on,"
            " Params %s not allowed to set in single operator model if instr_profiling is on.",
            comflictStr.c_str());
        return true;
    }
    return false;
}

bool InputParser::CheckInputDataValidity(int argc, CONST_CHAR_PTR argv[])
{
    if (params_ == nullptr) {
        MSPROF_LOGE("params_ is null in InputParser.");
        return false;
    }

    if (argc > INPUT_MAX_LENTH || argv == nullptr) {
        CmdLog::instance()->CmdErrorLog("input data is invalid,"
            "please input argc less than %d and argv is not null", INPUT_MAX_LENTH);
        return false;
    }
    for (int i = 0; i < argc; i++) {
        if (strnlen(argv[i], INPUT_MAX_LENTH) == INPUT_MAX_LENTH) {
            CmdLog::instance()->CmdErrorLog("input data is invalid,"
                "please input the len of every argv less than %d", INPUT_MAX_LENTH);
            return false;
        }
    }
    return true;
}

SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> InputParser::MsprofGetOpts(int argc, CONST_CHAR_PTR argv[])
{
    if (!CheckInputDataValidity(argc, argv)) {
        return nullptr;
    }

    int32_t argCount = 1;       // argv[0] is msprof
    std::string appStr = GetApplicationArgv(argc, argv, argCount);

    int opt = 0;
    int optionIndex = 0;
    MsprofString optString  = "";
    struct MsprofCmdInfo cmdInfo = { {nullptr} };
    std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> argvMap;
    std::string argvStr = "";
    while ((opt = MmGetOptLong(argCount, const_cast<MsprofStrBufAddrT>(argv),
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
        argvStr = std::string(argv[MmGetOptInd() - 1]);
        cmdInfo.args[opt] = MmGetOptArg();
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

    SetApplicationArgv(appStr, cmdInfo, argvMap);
    if (CheckInstrAndTaskParamBothSet(argvMap)) {
        return nullptr;
    }

    if (ParamsAdapterMsprof().GetParamFromInputCfg(argvMap, params_) != PROFILING_SUCCESS) {
        MSPROF_LOGE("[MsprofGetOpts]get param from InputCfg failed.");
        return nullptr;
    }
    return params_;
}

void InputParser::SetApplicationArgv(const std::string &appStr, struct MsprofCmdInfo &cmdInfo,
                                     std::unordered_map<int, std::pair<MsprofCmdInfo, std::string>> &argvMap)
{
    if (appStr.empty()) {
        params_->is_shell = false;
        return;
    }
    if (argvMap.find(ARGS_APPLICATION) != argvMap.end()) {
        params_->is_shell = false;
        CmdLog::instance()->CmdWarningLog("The executable file '%s' is useless when --application is not empty",
                                          appStr.c_str());
        return;
    }
    params_->is_shell = true;
    g_appStr = appStr;
    cmdInfo.args[ARGS_APPLICATION] = const_cast<CHAR_PTR>(g_appStr.c_str());
    std::string appArg = "--application=";
    argvMap[ARGS_APPLICATION] = std::pair<MsprofCmdInfo, std::string>(cmdInfo, appArg + appStr);

    if (argvMap.find(ARGS_OUTPUT) == argvMap.end()) {
        g_appPath = "./";
        cmdInfo.args[ARGS_OUTPUT] = const_cast<CHAR_PTR>(g_appPath.c_str());
        std::string outputStr = "--output=./";
        argvMap[ARGS_OUTPUT] = std::pair<MsprofCmdInfo, std::string>(cmdInfo, outputStr);
    }
}

/**
 * @brief find msprof command parameter and user parameter splits
 * @param [in] argc: argc
 * @param [in] argv: argv
 * @param [out] argCount: msprof parameter count
 */
std::string InputParser::GetApplicationArgv(int32_t argc, CONST_CHAR_PTR argv[], int32_t &argCount)
{
    std::stringstream sstr;
    const int32_t argWithSpaceNum = 2;
    for (int32_t i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            if (std::strchr(argv[i], '=') != nullptr) {
                argCount++;
            } else {
                argCount += argWithSpaceNum;
                i++;
            }
        } else {
            for (;i < argc; i++) {
                sstr << argv[i] << " ";
            }
        }
    }
    return sstr.str();
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

ArgsManager::ArgsManager() : driverOnline_(false), platform_(PlatformType::END_TYPE)
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
    AddBasicArgs();
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
    AddInstrArgs();
    AddHostArgs();
    AddStarsArgs();
    Args help = {"help", "help message.(full-platform)"};
    argsList_.push_back(help);
}

void ArgsManager::AddBasicArgs()
{
    argsList_ = {
        {"output", "Specify the directory that is used for storing data results.(full-platform)"},
        {"storage-limit", "Specify the output directory volume. range 200MB ~ 4294967295MB.(full-platform)"},
        {"application", "Specify application path, considering the risk of privilege escalation,\n"
            "\t\t\t\t\t\t   please pay attention to the group of the application and \n"
            "\t\t\t\t\t\t   confirm whether it is the same as the user currently.(full-platform)"},
        {"ascendcl", "Show acl profiling data, the default value is on.(full-platform)", ON},
        {"model-execution", "Show ge model execution profiling data, the default value is off.(full-platform)", OFF},
        {"runtime-api", "Show runtime api profiling data, the default value is off.(full-platform)", OFF},
         {"task-time", "Show task profiling data, the default value is on.(full-platform)\n"
            "\t\t\t\t\t\t   The switch can be set in range [l0, l1, on, off]", ON},
        {"environment", "User app custom environment variable configuration.(full-platform)"},
        {"sys-period", "Set total sampling period of system profiling in seconds.(full-platform)"},
        {"sys-devices", "Specify the profiling scope by device ID when collect sys profiling.\n"
            "\t\t\t\t\t\t   The value is all or ID list (split with ',').(full-platform)"},
        {"hccl", "Show hccl profiling data, the default value is off.(full-platform)", OFF},
        {"msproftx", "Show msproftx data, the default value is off.(full-platform)", OFF},
        {"ai-core", "Turn on / off the ai core profiling, the default value is on when collecting\n"
            "\t\t\t\t\t\t   app Profiling.(full-platform)", ON},
        {"aic-mode", "Set the aic profiling mode to task-based or sample-based.(full-platform)\n"
            "\t\t\t\t\t\t   In task-based mode, profiling data will be collected by tasks.\n"
            "\t\t\t\t\t\t   In sample-based mode, profiling data will be collected in a specific interval.\n"
            "\t\t\t\t\t\t   The default value is task-based.", TASK_BASED},
        {"aic-freq", "The aic sampling frequency in hertz, the default value is 100 Hz, the range is\n"
            "\t\t\t\t\t\t   1 to 100 Hz.(full-platform)", "100"},
        {"task-memory", "Show operator memory data, the default value is off.(full-platform)", OFF}
    };
    std::string defaultOption = (platform_ == PlatformType::CHIP_V4_2_0) ? "PipelineExecuteUtilization" :
                                "PipeUtilization";
    std::string addOptions;
    if (!driverOnline_ || platform_ == PlatformType::CHIP_V4_2_0) {
        addOptions = ", L2Cache, PipelineExecuteUtilization";
    } else if (platform_ == PlatformType::CHIP_V4_1_0) {
        addOptions = ", L2Cache";
    }
    Args aicMetrics = {"aic-metrics",
                       "The aic metrics groups, include ArithmeticUtilization, PipeUtilization, Memory,\n"
                           "\t\t\t\t\t\t   MemoryL0, ResourceConflictRatio, MemoryUB" + addOptions +".\n"
                           "\t\t\t\t\t\t   You can also customize registers in the format of Custom:xx,xx.\n"
                           "\t\t\t\t\t\t   The default value is " + defaultOption + ".(full-platform)",
                       defaultOption};
    argsList_.push_back(aicMetrics);
}

void ArgsManager::AddDynProfArgs()
{
    if (driverOnline_ &&
        (platform_ != PlatformType::CLOUD_TYPE && platform_ != PlatformType::CHIP_V4_1_0 &&
            platform_ != PlatformType::CHIP_V4_2_0 && platform_ != PlatformType::DC_TYPE)) {
        return;
    }
    Args dynamic = {"dynamic", "Dynamic profiling switch, the default value is off."
                    "(Ascend910, Ascend910B, Ascend910_93, Ascend310B, Ascend310P)",
                    OFF};
    Args pid = {"pid", "Dynamic profiling pid of the target process."
                "(Ascend910, Ascend910B, Ascend910_93, Ascend310B, Ascend310P)",
                "0"};
    Args delay = {"delay",
        "Collect start delay time in seconds, range 1 ~ 4294967295s."
            "(Ascend910, Ascend910B, Ascend910_93, Ascend310B, Ascend310P)"};
    Args duration = {"duration",
        "Collection duration in seconds, range 1 ~ 4294967295s."
            "(Ascend910, Ascend910B, Ascend910_93, Ascend310B, Ascend310P)"};
    argsList_.push_back(dynamic);
    argsList_.push_back(pid);
    argsList_.push_back(delay);
    argsList_.push_back(duration);
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
        {"analyze", "Switch for using msprof to analyze collecting data,\n"
            "\t\t\t\t\t\t   the default value is off.(full-platform)", OFF},
        {"rule", "Switch specified rule for using msprof to analyze collecting data,\n"
            "\t\t\t\t\t\t   the default value is communication,communication_matrix.(full-platform)\n"
            "\t\t\t\t\t\t   The switch can be set in [communication, communication_matrix]"},
        {"parse", "Switch for using msprof to parse collecting data, the default value\n"
            "\t\t\t\t\t\t   is off.(full-platform)", OFF},
        {"query", "Switch for using msprof to query collecting data, the default value\n"
            "\t\t\t\t\t\t   is off.(full-platform)", OFF},
        {"export", "Switch for using msprof to export collecting data, the default value\n"
            "\t\t\t\t\t\t   is off.(full-platform)", OFF},
        {"clear", "Swith for using msprof to analyze or export data in clear mode, the default value\n"
            "\t\t\t\t\t\t   is off.(full-platform)", OFF},
        {"iteration-id", "The export iteration id, only uesd when argument export is on,\n"
            "\t\t\t\t\t\t   the default value is 1", "1"},
        {"model-id", "The export model id, only uesd when argument export is on, msprof will\n"
            "\t\t\t\t\t\t   export minium accessible model by default.(full-platform)", "-1"},
        {"type", "The export type, only used when the parameters `application` or `export` \n"
            "\t\t\t\t\t\t   are set to on or when it is a system-collected value.\n"
            "\t\t\t\t\t\t   include db, text, the default value is text.(full-platform)", "text"},
        {"summary-format", "The export summary file format, only uesd when argument export is on,\n"
            "\t\t\t\t\t\t   include csv, json, the default value is csv.(full-platform)", "csv"}
    };
    argsList_.insert(argsList_.end(), argsList.begin(), argsList.end());
}

void ArgsManager::AddStarsArgs()
{
    if (driverOnline_) {
        return;
    }

    Args lowPowerArgs = {"power", "Show low power profiling data, the default value is off.(future-platform)"};
    argsList_.push_back(lowPowerArgs);
}

void ArgsManager::AddInstrArgs()
{
    if (driverOnline_ && platform_ != PlatformType::CHIP_V4_1_0) {
        return;
    }
    Args instr = {"instr-profiling", "Show instr profiling data, the default value is off.(Ascend910B, Ascend910_93)",
        OFF};
    Args instrFreq = {"instr-profiling-freq",
                      "The instr sampling period in clock-cycle, the default value is 1000 cycle,\n"
                          "\t\t\t\t\t\t   the range is 300 to 30000 cycle.(Ascend910B, Ascend910_93)",
                      "1000"};
    argsList_.push_back(instr);
    argsList_.push_back(instrFreq);
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
                              "\t\t\t\t\t\t   LLC bandwidth(Ascend310 while set llc-profiling to bandwidth).\n"
                              "\t\t\t\t\t\t   LLC usage by aicpu and control cpu(Ascend310 while set llc-profiling to"
                              " capacity).\n"
                              "\t\t\t\t\t\t   LLC read/write speed(Ascend310P, Ascend910, Ascend310B, "
                              "Ascend910B, Ascend910_93).\n"
                              "\t\t\t\t\t\t   DDR read/write speed(Ascend310, Ascend310P, Ascend910, Ascend310B).\n"
                              "\t\t\t\t\t\t   HBM read/write speed(Ascend910, Ascend910B, Ascend910_93).\n"
                              "\t\t\t\t\t\t   Processes-level and system-level device memory usage(full-platform)");
    hardwareMemFreq.SetDetail("LLC, DDR, HBM acquisition frequency, range 1 ~ 100, the default value is 50 Hz");
    llcMode.SetDetail("The llc profiling groups.\n"
                            "\t\t\t\t\t\t   include capacity, bandwidth. the default value is capacity.(Ascend310)\n"
                            "\t\t\t\t\t\t   include read, write. the default value is read.\n"
                            "\t\t\t\t\t\t   (Ascend310P, Ascend910, Ascend310B, Ascend910B, Ascend910_93)");
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
                        "\t\t\t\t\t\t   NIC(Ascend310, Ascend910, Ascend310B, Ascend910B, Ascend910_93), "
                        "ROCE(Ascend910, Ascend910B, Ascend910_93)",
                   OFF};
    Args ioFreqArgs = {"sys-io-sampling-freq",
                       "NIC ROCE acquisition frequency, range 1 ~ 100, the default value is 100 Hz.\n"
                            "\t\t\t\t\t\t   NIC(Ascend310, Ascend910, Ascend310B, Ascend910B, Ascend910_93), "
                            "ROCE(Ascend910, Ascend910B, Ascend910_93)",
                       "100"};
    argsList_.push_back(ioArgs);
    argsList_.push_back(ioFreqArgs);
}

void ArgsManager::AddInterArgs()
{
    if (driverOnline_ && (platform_ == PlatformType::LHISI_TYPE || platform_ == PlatformType::MINI_TYPE ||
        platform_ == PlatformType::MDC_TYPE || platform_ == PlatformType::CHIP_V4_2_0)) {
        return;
    }
    Args interArgs = {"sys-interconnection-profiling",
                      "PCIE, HCCS acquisition switch, the default value is off.\n"
                          "\t\t\t\t\t\t   PCIE(Ascend310P, Ascend910, Ascend910B, Ascend910_93), "
                          "HCCS(Ascend910, Ascend910B, Ascend910_93)",
                      OFF};
    Args interFreq = {"sys-interconnection-freq",
                      "PCIE, HCCS acquisition frequency, range 1 ~ 50, the default value is 50 Hz.\n"
                          "\t\t\t\t\t\t   PCIE(Ascend310P, Ascend910, Ascend910B, Ascend910_93), "
                          "HCCS(Ascend910, Ascend910B, Ascend910_93)",
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
    Args l2 = {"l2",
               "L2 Cache acquisition switch. the default value is off.\n"
                   "\t\t\t\t\t\t   (Ascend310P, Ascend910, Ascend310B, Ascend910B, Ascend910_93)",
               OFF};
    argsList_.push_back(l2);
}

static void PrintOp()
{
    std::cout << "This is subcommand for operator optimization situation:" << std::endl;
    const int optionSpace = 34;
    std::cout << "      "; // 6 space
    std::cout << std::left << std::setw(optionSpace) << "op";
    std::cout << "Use binary msopprof to operator optimization (msprof op ...)" << std::endl << std::endl;
}

void ArgsManager::PrintHelp()
{
    Init();
    std::cout << "msprof (MindStudio Profiler) is part of performance analysis tools powered by MindStudio.";
    std::cout << std::endl << "Usage:" << std::endl;
    std::cout << "      ./msprof [--options]" << std::endl << std::endl;
    PrintOp();
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