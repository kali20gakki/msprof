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
    return ParamsCheck() == MSPROF_DAEMON_OK ? params_ : nullptr;
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

int InputParser::ProcessOptions(int opt, struct MsprofCmdInfo &cmdInfo)
{
    int ret = MSPROF_DAEMON_ERROR;
    // opt range validate
    if (opt < ARGS_HELP || opt >= NR_ARGS) {
        MsprofCmdUsage("");
        return ret;
    }
    cmdInfo.args[opt] = MmGetOptArg();
    params_->usedParams.insert(opt);

    if (opt >= ARGS_OUTPUT && opt <= ARGS_SUMMARY_FORMAT) {
        ret = MsprofCmdCheckValid(cmdInfo, opt);
    } else if (opt >= ARGS_ASCENDCL && opt <= ARGS_EXPORT) {
        ret = MsprofSwitchCheckValid(cmdInfo, opt);
    } else if (opt >= ARGS_AIC_FREQ && opt <= ARGS_EXPORT_MODEL_ID) {
        ret = MsprofFreqCheckValid(cmdInfo, opt);
    } else if (opt >= ARGS_HOST_SYS && opt <= ARGS_HOST_USAGE) {
        ret = MsprofHostCheckValid(cmdInfo, opt);
    } else {
        // when opt matches ARGS_HELP
        MsprofCmdUsage("");
        ret = MSPROF_DAEMON_OK;
    }
    return ret;
}

int InputParser::ParamsCheck() const
{
    if (params_ == nullptr) {
        return MSPROF_DAEMON_ERROR;
    }
    if (params_->result_dir.empty() && !params_->app_dir.empty()) {
        params_->result_dir = params_->app_dir;
    }
    return MSPROF_DAEMON_OK;
}

/**
 * @brief check host-sys parameter is valid or not
 * @param cmd_info: command info
 *
 * @return
 *        MSPROF_DAEMON_OK: succ
 *        MSPROF_DAEMON_ERROR: failed
 */
int InputParser::CheckHostSysValid(const struct MsprofCmdInfo &cmdInfo)
{
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    CmdLog::instance()->CmdErrorLog("Currently, --host-sys can be used only in the Linux environment.");
#endif
    if (Platform::instance()->RunSocSide()) {
        CmdLog::instance()->CmdErrorLog("Not in host side, --host-sys is not supported");
    }
    if (cmdInfo.args[ARGS_HOST_SYS] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --host-sys: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    std::string hostSys = std::string(cmdInfo.args[ARGS_HOST_SYS]);
    if (hostSys.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --host-sys is empty. Please input in the range of "
            "'cpu|mem|disk|network|osrt'");
        return MSPROF_DAEMON_ERROR;
    }
    std::vector<std::string> hostSysArray = Utils::Split(cmdInfo.args[ARGS_HOST_SYS], false, "", ",");
    for (size_t i = 0; i < hostSysArray.size(); ++i) {
        if (!(ParamValidation::instance()->CheckHostSysOptionsIsValid(hostSysArray[i]))) {
            CmdLog::instance()->CmdErrorLog("Argument --host-sys: invalid value:%s. Please input in the range of "
                "'cpu|mem|disk|network|osrt'", hostSysArray[i].c_str());
            return MSPROF_DAEMON_ERROR;
        }
        SetHostSysParam(hostSysArray[i]);
    }
    if (params_->host_osrt_profiling.compare(ON) == 0) {
        if (CheckHostSysToolsIsExist(TOOL_NAME_PERF) != MSPROF_DAEMON_OK) {
            CmdLog::instance()->CmdErrorLog("The tool perf is invalid, please check"
                " if the tool and sudo are available.");
            return MSPROF_DAEMON_ERROR;
        }
        if (CheckHostSysToolsIsExist(TOOL_NAME_LTRACE) != MSPROF_DAEMON_OK) {
            CmdLog::instance()->CmdErrorLog("The tool ltrace is invalid, please check"
                " if the tool and sudo are available.");
            return MSPROF_DAEMON_ERROR;
        }
    }
    if (params_->host_disk_profiling.compare(ON) == 0) {
        if (CheckHostSysToolsIsExist(TOOL_NAME_IOTOP) != MSPROF_DAEMON_OK) {
            CmdLog::instance()->CmdErrorLog("The tool iotop is invalid, please check if"
                " the tool and sudo are available.");
            return MSPROF_DAEMON_ERROR;
        }
    }
    params_->host_sys = cmdInfo.args[ARGS_HOST_SYS];
    return MSPROF_DAEMON_OK;
}

void InputParser::SetHostSysParam(const std::string hostSysParam)
{
    if (hostSysParam.compare(HOST_SYS_CPU) == 0) {
        params_->host_cpu_profiling = ON;
    } else if (hostSysParam.compare(HOST_SYS_MEM) == 0) {
        params_->host_mem_profiling = ON;
    } else if (hostSysParam.compare(HOST_SYS_NETWORK) == 0) {
        params_->host_network_profiling = ON;
    } else if (hostSysParam.compare(HOST_SYS_DISK) == 0) {
        params_->host_disk_profiling = ON;
    } else if (hostSysParam.compare(HOST_SYS_OSRT) == 0) {
        params_->host_osrt_profiling = ON;
    }
}

int InputParser::CheckHostSysToolsIsExist(const std::string toolName)
{
    if (params_->result_dir.empty() && params_->app_dir.empty()) {
        CmdLog::instance()->CmdErrorLog("If you want to use this parameter:--host-sys,"
            "please put it behind the --output or --application. ");
        return MSPROF_DAEMON_ERROR;
    }
    MSPROF_LOGI("Start the detection tool.");
    std::string tmpDir;
    if (!params_->result_dir.empty()) {
        tmpDir = params_->result_dir;
    } else if (!params_->app_dir.empty()) {
        tmpDir = params_->app_dir;
    }
    static const std::string ENV_PATH = "PATH=/usr/bin/:/usr/sbin:/var";
    std::vector<std::string> envV;
    envV.push_back(ENV_PATH);
    std::vector<std::string> argsV;
    argsV.push_back(PROF_SCRIPT_FILE_PATH);
    argsV.push_back("get-version");
    argsV.push_back(toolName);
    unsigned long long startRealtime = analysis::dvvp::common::utils::Utils::GetClockRealtime();
    tmpDir += "/tmpPrint" + std::to_string(startRealtime);
    int exitCode = analysis::dvvp::common::utils::INVALID_EXIT_CODE;
    static const std::string CMD = "sudo";
    mmProcess tmpProcess = MSVP_MMPROCESS;
    ExecCmdParams execCmdParams(CMD, true, tmpDir);
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams,
                                                            argsV,
                                                            envV,
                                                            exitCode,
                                                            tmpProcess);
    FUNRET_CHECK_FAIL_PRINT(ret, PROFILING_SUCCESS);
    ret = CheckHostSysCmdOutIsExist(tmpDir, toolName, tmpProcess);
    return ret;
}

int InputParser::CheckHostSysCmdOutIsExist(const std::string tmpDir, const std::string toolName,
                                           const mmProcess tmpProcess)
{
    MSPROF_LOGI("Start to check whether the file exists.");
    for (int i = 0; i < FILE_FIND_REPLAY; i++) {
        if (!(Utils::IsFileExist(tmpDir))) {
            MmSleep(20); // If the file is not found, the delay is 20 ms.
            continue;
        } else {
            break;
        }
    }
    for (int i = 0; i < FILE_FIND_REPLAY; i++) {
        long long len = analysis::dvvp::common::utils::Utils::GetFileSize(tmpDir);
        if (len < static_cast<int>(toolName.length())) {
            MmSleep(5); // If the file has no content, the delay is 5 ms.
            continue;
        } else {
            break;
        }
    }
    std::ifstream in(tmpDir);
    std::ostringstream tmp;
    tmp << in.rdbuf();
    std::string tmpStr = tmp.str();
    MmUnlink(tmpDir);
    int ret = CheckHostOutString(tmpStr, toolName);
    if (ret != MSPROF_DAEMON_OK) {
        ret = UninitCheckHostSysCmd(tmpProcess); // stop check process.
        if (ret != MSPROF_DAEMON_OK) {
            MSPROF_LOGE("Failed to kill the process.");
        }
        MSPROF_LOGE("The tool %s useless", toolName.c_str());
        return MSPROF_DAEMON_ERROR;
    }
    return ret;
}

/**
 * Check the first word in the string is tool name.
 */
int InputParser::CheckHostOutString(const std::string tmpStr, const std::string toolName)
{
    std::vector<std::string> checkToolArray = Utils::Split(tmpStr.c_str());
    if (checkToolArray.size() > 0) {
        if (checkToolArray[0].compare(toolName) == 0) {
            MSPROF_LOGI("The returned value is correct.%s", checkToolArray[0].c_str());
            return MSPROF_DAEMON_OK;
        } else {
            MSPROF_LOGE("The return value is incorrect.%s", checkToolArray[0].c_str());
            return MSPROF_DAEMON_ERROR;
        }
    }
    MSPROF_LOGE("The file has no content.");
    return MSPROF_DAEMON_ERROR;
}

int InputParser::UninitCheckHostSysCmd(const mmProcess checkProcess)
{
    if (!(ParamValidation::instance()->CheckHostSysPidIsValid(reinterpret_cast<int>(checkProcess)))) {
        return MSPROF_DAEMON_ERROR;
    }
    if (!analysis::dvvp::common::utils::Utils::ProcessIsRuning(checkProcess)) {
        MSPROF_LOGI("Process:%d is not exist", reinterpret_cast<int>(checkProcess));
        return MSPROF_DAEMON_OK;
    }
    static const std::string ENV_PATH = "PATH=/usr/bin/:/usr/sbin:/var:/bin";
    std::vector<std::string> envV;
    envV.push_back(ENV_PATH);
    std::vector<std::string> argsV;
    std::string killCmd = "kill -2 " + std::to_string(reinterpret_cast<int>(checkProcess));
    argsV.push_back("-c");
    argsV.push_back(killCmd);
    int exitCode = analysis::dvvp::common::utils::VALID_EXIT_CODE;
    static const std::string CMD = "sh";
    mmProcess tmpProcess = MSVP_MMPROCESS;
    ExecCmdParams execCmdParams(CMD, true, "");
    int ret = MSPROF_DAEMON_OK;
    for (int i = 0; i < FILE_FIND_REPLAY; i++) {
        if (ParamValidation::instance()->CheckHostSysPidIsValid(reinterpret_cast<int>(checkProcess))) {
            ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envV, exitCode, tmpProcess);
            MmSleep(20); // If failed stop check process, the delay is 20 ms.
            continue;
        } else {
            break;
        }
    }
    if (checkProcess > 0) {
        bool isExited = false;
        ret = analysis::dvvp::common::utils::Utils::WaitProcess(checkProcess,
                                                                isExited,
                                                                exitCode,
                                                                true);
        if (ret != PROFILING_SUCCESS) {
            ret = MSPROF_DAEMON_ERROR;
            MSPROF_LOGE("Failed to wait process %d, ret=%d",
                        reinterpret_cast<int>(checkProcess), ret);
        } else {
            ret = MSPROF_DAEMON_OK;
            MSPROF_LOGI("Process %d exited, exit code=%d",
                        reinterpret_cast<int>(checkProcess), exitCode);
        }
    }
    return ret;
}

int InputParser::CheckHostSysPidValid(const struct MsprofCmdInfo &cmdInfo)
{
    if (cmdInfo.args[ARGS_HOST_SYS_PID] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --host-sys-pid is empty,"
            "Please enter a valid --host-sys-pid value.");
        return MSPROF_DAEMON_ERROR;
    }

    if (Utils::CheckStringIsNonNegativeIntNum(cmdInfo.args[ARGS_HOST_SYS_PID])) {
        auto hostSysRet = std::stoi(cmdInfo.args[ARGS_HOST_SYS_PID]);
        if (!(ParamValidation::instance()->CheckHostSysPidIsValid(hostSysRet))) {
            CmdLog::instance()->CmdErrorLog("Argument --host-sys-pid: invalid int value: %d."
                "The process cannot be found, please enter a correct host-sys-pid.", hostSysRet);
            return MSPROF_DAEMON_ERROR;
        } else {
            params_->host_sys_pid = std::stoi(cmdInfo.args[ARGS_HOST_SYS_PID]);
            return MSPROF_DAEMON_OK;
        }
    } else {
        CmdLog::instance()->CmdErrorLog("Argument --host-sys-pid: invalid value: %s."
            "Please input an integer value.The min value is 0.", cmdInfo.args[ARGS_HOST_SYS_PID]);
        return MSPROF_DAEMON_ERROR;
    }
}

int InputParser::CheckOutputValid(const struct MsprofCmdInfo &cmdInfo)
{
    if (cmdInfo.args[ARGS_OUTPUT] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --output: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    std::string path = Utils::RelativePathToAbsolutePath(cmdInfo.args[ARGS_OUTPUT]);
    if (!path.empty()) {
        if (path.size() > MAX_PATH_LENGTH) {
            CmdLog::instance()->CmdErrorLog("Argument --output is invalid because of exceeds"
                " the maximum length of %d", MAX_PATH_LENGTH);
            return MSPROF_DAEMON_ERROR;
        }
        if (Utils::CreateDir(path) != PROFILING_SUCCESS) {
            char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
            CmdLog::instance()->CmdErrorLog("Create output dir failed.ErrorCode: %d, ErrorInfo: %s.",
                MmGetErrorCode(),
                MmGetErrorFormatMessage(MmGetErrorCode(),
                    errBuf, MAX_ERR_STRING_LEN));
            return MSPROF_DAEMON_ERROR;
        }
        if (!Utils::IsDir(path)) {
            CmdLog::instance()->CmdErrorLog("Argument --output %s is not a dir.", params_->result_dir.c_str());
            return MSPROF_DAEMON_ERROR;
        }
        if (MmAccess2(path, M_W_OK) != PROFILING_SUCCESS) {
            CmdLog::instance()->CmdErrorLog("Argument --output %s permission denied.", params_->result_dir.c_str());
            return MSPROF_DAEMON_ERROR;
        }
        params_->result_dir = Utils::CanonicalizePath(path);
        if (params_->result_dir.empty()) {
            CmdLog::instance()->CmdErrorLog("Argument --output is invalid because of"
                " get the canonicalized absolute pathname failed");
            return MSPROF_DAEMON_ERROR;
        }
    } else {
        CmdLog::instance()->CmdErrorLog("Argument --output: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    return MSPROF_DAEMON_OK;
}

int InputParser::CheckStorageLimitValid(const struct MsprofCmdInfo &cmdInfo) const
{
    if (cmdInfo.args[ARGS_STORAGE_LIMIT] == nullptr) {
        return MSPROF_DAEMON_OK;
    }
    params_->storageLimit = cmdInfo.args[ARGS_STORAGE_LIMIT];
    if (params_->storageLimit.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --storage-limit: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    if (!ParamValidation::instance()->CheckStorageLimit(params_->storageLimit)) {
        CmdLog::instance()->CmdErrorLog("Argument --storage-limit %s invalid", params_->storageLimit.c_str());
        return MSPROF_DAEMON_ERROR;
    }
    return MSPROF_DAEMON_OK;
}

int InputParser::GetAppParam(const std::string &appParams)
{
    if (appParams.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --application: expected one script");
        return MSPROF_DAEMON_ERROR;
    }
    size_t index = appParams.find_first_of(" ");
    if (index != std::string::npos) {
        params_->app_parameters = appParams.substr(index + 1);
    }
    std::string appPath = appParams.substr(0, index);
    appPath = Utils::CanonicalizePath(appPath);
    if (appPath.empty()) {
        CmdLog::instance()->CmdErrorLog("Script params are invalid");
        return MSPROF_DAEMON_ERROR;
    }

    if (Utils::IsSoftLink(appPath)) {
        MSPROF_LOGE("Script(%s) is soft link.", Utils::BaseName(appPath).c_str());
        return MSPROF_DAEMON_ERROR;
    }
    std::string appDir;
    std::string appName;
    int ret = Utils::SplitPath(appPath, appDir, appName);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to get app dir");
        return MSPROF_DAEMON_ERROR;
    }
    
    params_->app_dir = appDir;
    params_->app = appName;
    return MSPROF_DAEMON_OK;
}

int InputParser::CheckAppValid(const struct MsprofCmdInfo &cmdInfo)
{
    if (cmdInfo.args[ARGS_APPLICATION] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --application: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    std::string appParam = cmdInfo.args[ARGS_APPLICATION];
    if (appParam.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --application: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    std::string tmpAppParamers;
    size_t index = appParam.find_first_of(" ");
    if (index != std::string::npos) {
        tmpAppParamers = appParam.substr(index + 1);
    }
    std::string cmdPath = appParam.substr(0, index);
    if (!Utils::IsAppName(cmdPath) && cmdPath.find("/") == std::string::npos) {
        params_->cmdPath = cmdPath;
        return GetAppParam(tmpAppParamers);
    }
    cmdPath = Utils::RelativePathToAbsolutePath(cmdPath);
    if (!Utils::IsAppName(cmdPath)) {
        if (Utils::CanonicalizePath(cmdPath).empty()) {
            CmdLog::instance()->CmdErrorLog("App path(%s) does not exist or permission denied.", cmdPath.c_str());
            return MSPROF_DAEMON_ERROR;
        }
        if (MmAccess2(cmdPath, M_X_OK) != PROFILING_SUCCESS) {
            CmdLog::instance()->CmdErrorLog("This app(%s) has no executable permission.", cmdPath.c_str());
            return MSPROF_DAEMON_ERROR;
        }
        params_->cmdPath = cmdPath;
        return GetAppParam(tmpAppParamers);
    }
    params_->app_parameters = tmpAppParamers;
    std::string cmdDir;
    std::string cmdName;
    int ret = Utils::SplitPath(cmdPath, cmdDir, cmdName);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to get cmd dir");
        return MSPROF_DAEMON_ERROR;
    }
    ret = PreCheckApp(cmdDir, cmdName);
    if (ret == MSPROF_DAEMON_OK) {
        params_->app_dir = cmdDir;
        params_->app = cmdName;
        params_->cmdPath = cmdPath;
        return MSPROF_DAEMON_OK;
    }
    return MSPROF_DAEMON_ERROR;
}

/**
 * Check validation of app name and app dir before launch.
 */
int InputParser::PreCheckApp(const std::string &appDir, const std::string &appName)
{
    if (appDir.empty() || appName.empty()) {
        return MSPROF_DAEMON_ERROR;
    }
    // check app name
    if (!ParamValidation::instance()->CheckAppNameIsValid(appName)) {
        CmdLog::instance()->CmdErrorLog("Argument --application(%s) is invalid, appName.", appName.c_str());
        return MSPROF_DAEMON_ERROR;
    }
    // check app path
    const std::string appPath = appDir + MSVP_SLASH + appName;
    if (Utils::IsSoftLink(appPath)) {
        CmdLog::instance()->CmdErrorLog("App path(%s) is soft link, not support!", appPath.c_str());
        return MSPROF_DAEMON_ERROR;
    }
    if (Utils::CanonicalizePath(appPath).empty()) {
        CmdLog::instance()->CmdErrorLog("App path(%s) does not exist or permission denied!!!", appPath.c_str());
        return MSPROF_DAEMON_ERROR;
    }
    // check app permisiion
    if (MmAccess2(appPath, M_X_OK) != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("This app(%s) has no executable permission!", appPath.c_str());
        return MSPROF_DAEMON_ERROR;
    }

    if (Utils::IsDir(appPath)) {
        CmdLog::instance()->CmdErrorLog("Argument --application:%s is a directory, "
            "please enter the executable file path.", appPath.c_str());
        return MSPROF_DAEMON_ERROR;
    }
    return MSPROF_DAEMON_OK;
}


int InputParser::CheckEnvironmentValid(const struct MsprofCmdInfo &cmdInfo)
{
    if (cmdInfo.args[ARGS_ENVIRONMENT] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --environment: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    params_->app_env = cmdInfo.args[ARGS_ENVIRONMENT];
    if (params_->app_env.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --environment: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    return MSPROF_DAEMON_OK;
}

int InputParser::CheckSampleModeValid(const struct MsprofCmdInfo &cmdInfo, int opt) const
{
    std::map<int, std::string> sampleMap = {
        {ARGS_AIC_MODE, "--aic-mode"},
        {ARGS_AIV_MODE, "--aiv-mode"}
    };

    if (cmdInfo.args[opt] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument %s: expected one argument", sampleMap[opt].c_str());
        return MSPROF_DAEMON_ERROR;
    }

    if (std::string(cmdInfo.args[opt]) == TASK_BASED ||
        std::string(cmdInfo.args[opt]) == SAMPLE_BASED) {
        params_->aiv_profiling_mode = (opt == ARGS_AIV_MODE) ?
            cmdInfo.args[ARGS_AIV_MODE] : params_->aiv_profiling_mode;
        params_->ai_core_profiling_mode = (opt == ARGS_AIC_MODE) ?
            cmdInfo.args[ARGS_AIC_MODE] : params_->ai_core_profiling_mode;
        return MSPROF_DAEMON_OK;
    } else {
        CmdLog::instance()->CmdErrorLog("Argument %s: invalid value: %s."
            "Please input 'task-based' or 'sample-based'.", sampleMap[opt].c_str(), cmdInfo.args[opt]);
        return MSPROF_DAEMON_ERROR;
    }
}

int InputParser::CheckPythonPathValid(const struct MsprofCmdInfo &cmdInfo) const
{
    if (cmdInfo.args[ARGS_PYTHON_PATH] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --python-path: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    params_->pythonPath = cmdInfo.args[ARGS_PYTHON_PATH];
    if (params_->pythonPath.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --python-path: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }

    if (params_->pythonPath.size() > MAX_PATH_LENGTH) {
        CmdLog::instance()->CmdErrorLog("Argument --python-path is invalid because of exceeds"
            " the maximum length of %d", MAX_PATH_LENGTH);
        return MSPROF_DAEMON_ERROR;
    }

    std::string absolutePythonPath = Utils::CanonicalizePath(params_->pythonPath);
    if (absolutePythonPath.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --python-path %s does not exist or permission denied!!!",
            params_->pythonPath.c_str());
        return MSPROF_DAEMON_ERROR;
    }

    if (MmAccess2(absolutePythonPath, M_X_OK) != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Argument --python-path %s permission denied.", params_->pythonPath.c_str());
        return MSPROF_DAEMON_ERROR;
    }

    if (Utils::IsDir(absolutePythonPath)) {
        CmdLog::instance()->CmdErrorLog("Argument --python-path %s is a directory, "
            "please enter the executable file path.", params_->pythonPath.c_str());
        return MSPROF_DAEMON_ERROR;
    }

    return MSPROF_DAEMON_OK;
}

int InputParser::CheckExportSummaryFormat(const struct MsprofCmdInfo &cmdInfo) const
{
    if (cmdInfo.args[ARGS_SUMMARY_FORMAT] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --summary-format: expected one argument");
        return MSPROF_DAEMON_ERROR;
    }
    params_->exportSummaryFormat = cmdInfo.args[ARGS_SUMMARY_FORMAT];
    if (params_->exportSummaryFormat != JSON_FORMAT && params_->exportSummaryFormat != CSV_FORMAT) {
        CmdLog::instance()->CmdErrorLog("Argument --summary-format: invalid value: %s. "
            "Please input 'json' or 'csv'.", cmdInfo.args[ARGS_SUMMARY_FORMAT]);
        return MSPROF_DAEMON_ERROR;
    }
    return MSPROF_DAEMON_OK;
}

int InputParser::CheckAiCoreMetricsValid(const struct MsprofCmdInfo &cmdInfo, int opt) const
{
    std::map<int, std::string> metricsMap = {
        {ARGS_AIC_METRICE, "--aic-metrics"},
        {ARGS_AIV_METRICS, "--aiv-metrics"},
    };

    if (cmdInfo.args[opt] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument %s: expected one argument", metricsMap[opt].c_str());
        return MSPROF_DAEMON_ERROR;
    }
    std::string aicoreMetrics = std::string(cmdInfo.args[opt]);
    if (aicoreMetrics.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument %s is empty. Please input in the range of "
            "'ArithmeticUtilization|PipeUtilization|"
            "Memory|MemoryL0|ResourceConflictRatio|MemoryUB'", metricsMap[opt].c_str());
        return MSPROF_DAEMON_ERROR;
    }
    if (!ParamValidation::instance()->CheckProfilingAicoreMetricsIsValid(aicoreMetrics)) {
        CmdLog::instance()->CmdErrorLog("Argument %s: invalid value:%s. Please input in the range of "
            "'ArithmeticUtilization|PipeUtilization|"
            "Memory|MemoryL0|ResourceConflictRatio|MemoryUB'", metricsMap[opt].c_str(), aicoreMetrics.c_str());
        return MSPROF_DAEMON_ERROR;
    }
    params_->ai_core_metrics = (opt == ARGS_AIC_METRICE) ? cmdInfo.args[opt] : params_->ai_core_metrics;
    params_->aiv_metrics = (opt == ARGS_AIV_METRICS) ? cmdInfo.args[opt] : params_->aiv_metrics;
    return MSPROF_DAEMON_OK;
}

int InputParser::CheckLlcProfilingIsValid(const std::string &llcProfiling)
{
    std::vector<std::string> llcProfilingWhiteList = {
        LLC_READ,
        LLC_WRITE
    };
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        llcProfilingWhiteList = {LLC_CAPACITY, LLC_BANDWIDTH};
    }
    if (llcProfiling.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --llc-profiling is empty."
            "Please input in the range of '%s|%s'",
            llcProfilingWhiteList[0].c_str(), llcProfilingWhiteList[1].c_str());
        return MSPROF_DAEMON_ERROR;
    }

    for (size_t j = 0; j < llcProfilingWhiteList.size(); j++) {
        if (llcProfiling.compare(llcProfilingWhiteList[j]) == 0) {
            return MSPROF_DAEMON_OK;
        }
    }

    CmdLog::instance()->CmdErrorLog("Argument --llc-profiling: invalid value: %s. "
        "Please input in the range of '%s|%s'", llcProfiling.c_str(),
        llcProfilingWhiteList[0].c_str(), llcProfilingWhiteList[1].c_str());
    return MSPROF_DAEMON_ERROR;
}

int InputParser::CheckLlcProfilingVaild(const struct MsprofCmdInfo &cmdInfo)
{
    if (cmdInfo.args[ARGS_LLC_PROFILING] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --llc-profiling: expected one argument.");
        return MSPROF_DAEMON_ERROR;
    }

    std::string llcProfiling = std::string(cmdInfo.args[ARGS_LLC_PROFILING]);
    if (CheckLlcProfilingIsValid(llcProfiling) != MSPROF_DAEMON_OK) {
        return MSPROF_DAEMON_ERROR;
    }
    params_->llc_profiling = llcProfiling;
    return MSPROF_DAEMON_OK;
}

/**
 * @brief check sys-period parameter is valid or not
 * @param cmd_info: command info
 *
 * @return
 *        MSPROF_DAEMON_OK: succ
 *        MSPROF_DAEMON_ERROR: failed
 */
int InputParser::CheckSysPeriodVaild(const struct MsprofCmdInfo &cmdInfo)
{
    if (cmdInfo.args[ARGS_SYS_PERIOD] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --sys-period is empty,"
            "Please enter a valid --sys-period value.");
        return MSPROF_DAEMON_ERROR;
    }

    if (Utils::CheckStringIsNonNegativeIntNum(cmdInfo.args[ARGS_SYS_PERIOD])) {
        auto syspeRet = std::stoi(cmdInfo.args[ARGS_SYS_PERIOD]);
        if (!(ParamValidation::instance()->IsValidSleepPeriod(syspeRet))) {
            CmdLog::instance()->CmdErrorLog("Argument --sys-period: invalid int value: %d."
                "The max period is 30 days.", syspeRet);
            return MSPROF_DAEMON_ERROR;
        } else {
            return MSPROF_DAEMON_OK;
        }
    } else {
        CmdLog::instance()->CmdErrorLog("Argument --sys-period: invalid value: %s."
            "Please input an integer value.The max period is 30 days.", cmdInfo.args[ARGS_SYS_PERIOD]);
        return MSPROF_DAEMON_ERROR;
    }
}

/**
 * @brief check sys-devices parameter is valid or not
 * @param cmd_info: command info
 *
 * @return
 *        MSPROF_DAEMON_OK: succ
 *        MSPROF_DAEMON_ERROR: failed
 */
int InputParser::CheckSysDevicesVaild(const struct MsprofCmdInfo &cmdInfo)
{
    if (cmdInfo.args[ARGS_SYS_DEVICES] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --sys-devices is empty,"
            "Please enter a valid --sys-devices value.");
        return MSPROF_DAEMON_ERROR;
    }
    params_->devices = cmdInfo.args[ARGS_SYS_DEVICES];
    if (params_->devices.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --sys-devices is empty,"
            "Please enter a valid --sys-devices value.");
        return MSPROF_DAEMON_ERROR;
    }
    if (std::string(cmdInfo.args[ARGS_SYS_DEVICES]) == ALL) {
        return MSPROF_DAEMON_OK;
    }

    std::vector<std::string> devices = Utils::Split(cmdInfo.args[ARGS_SYS_DEVICES], false, "", ",");
    for (size_t i = 0; i < devices.size(); ++i) {
        if (!(ParamValidation::instance()->CheckDeviceIdIsValid(devices[i]))) {
            CmdLog::instance()->CmdErrorLog("Argument --sys-devices: invalid value: %s."
                "Please input data is a id num.", devices[i].c_str());
            return MSPROF_DAEMON_ERROR;
        }
    }

    return MSPROF_DAEMON_OK;
}

int InputParser::CheckArgOnOff(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    if (cmdInfo.args[opt] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --%s: expected one argument,please enter a valid value.",
            longOptions[opt].name);
        return MSPROF_DAEMON_ERROR;
    }
    if (std::string(cmdInfo.args[opt]) != ON &&
        std::string(cmdInfo.args[opt]) != OFF) {
        CmdLog::instance()->CmdErrorLog("Argument --%s: invalid value: %s. "
            "Please input 'on' or 'off'.", longOptions[opt].name, cmdInfo.args[opt]);
        return MSPROF_DAEMON_ERROR;
    }
    return MSPROF_DAEMON_OK;
}

int InputParser::CheckArgRange(const struct MsprofCmdInfo &cmdInfo, int opt, int min, int max)
{
    if (cmdInfo.args[opt] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --%s is empty, please enter a valid value.", longOptions[opt].name);
        return MSPROF_DAEMON_ERROR;
    }
    if (Utils::CheckStringIsNonNegativeIntNum(cmdInfo.args[opt])) {
        int optRet = std::stoi(cmdInfo.args[opt]);
        if ((optRet >= min) && (optRet <= max)) {
            return MSPROF_DAEMON_OK;
        } else {
            CmdLog::instance()->CmdErrorLog("Argument --%s: invalid int value: %d."
                "Please input data is in %d to %d.", longOptions[opt].name, optRet, min, max);
            return MSPROF_DAEMON_ERROR;
        }
    } else {
        CmdLog::instance()->CmdErrorLog("Argument --%s: invalid value: %s."
            "Please input an integer value in %d-%d.", longOptions[opt].name, cmdInfo.args[opt], min, max);
        return MSPROF_DAEMON_ERROR;
    }
}

int InputParser::CheckArgsIsNumber(const struct MsprofCmdInfo &cmdInfo, int opt) const
{
    if (cmdInfo.args[opt] == nullptr) {
        CmdLog::instance()->CmdErrorLog("Argument --%s is empty, please enter a valid value.", longOptions[opt].name);
        return MSPROF_DAEMON_ERROR;
    }
    if (!Utils::CheckStringIsValidNatureNum(cmdInfo.args[opt])) {
        CmdLog::instance()->CmdErrorLog("Argument --%s: invalid value: %s."
            "Please input an integer value.", longOptions[opt].name, cmdInfo.args[opt]);
        return MSPROF_DAEMON_ERROR;
    }
    return MSPROF_DAEMON_OK;
}

void InputParser::ParamsSwitchValid(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    if (opt >= NR_ARGS) {
        return;
    }

    switch (opt) {
        case ARGS_ASCENDCL:
            params_->acl = cmdInfo.args[opt];
            break;
        case ARGS_MODEL_EXECUTION:
            params_->modelExecution = cmdInfo.args[opt];
            break;
        case ARGS_RUNTIME_API:
            params_->runtimeApi = cmdInfo.args[opt];
            break;
        case ARGS_TASK_TIME:
            SetTaskTimeSwitch(cmdInfo.args[opt]);
            break;
        case ARGS_AI_CORE:
            params_->ai_core_profiling = cmdInfo.args[opt];
            break;
        case ARGS_AIV:
            params_->aiv_profiling = cmdInfo.args[opt];
            break;
        case ARGS_CPU_PROFILING:
            params_->cpu_profiling = cmdInfo.args[opt];
            break;
        case ARGS_SYS_PROFILING:
            params_->sys_profiling = cmdInfo.args[opt];
            break;
        case ARGS_PID_PROFILING:
            params_->pid_profiling = cmdInfo.args[opt];
            break;
        case ARGS_HARDWARE_MEM:
            params_->hardware_mem = cmdInfo.args[opt];
            break;
        case ARGS_IO_PROFILING:
            params_->io_profiling = cmdInfo.args[opt];
            break;
        case ARGS_HCCL:
            params_->hcclTrace = cmdInfo.args[opt];
            break;
        default:
            ParamsSwitchValid2(cmdInfo, opt);
            break;
    }
}

void InputParser::SetTaskTimeSwitch(const std::string timeSwitch)
{
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        params_->ts_timeline = timeSwitch;
    } else {
        params_->hwts_log = timeSwitch;
        params_->hwts_log1 = timeSwitch;
    }
    params_->ts_memcpy = timeSwitch;
}

void InputParser::ParamsSwitchValid2(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    switch (opt) {
        case ARGS_INTERCONNECTION_PROFILING:
            params_->interconnection_profiling = cmdInfo.args[opt];
            break;
        case ARGS_DVPP_PROFILING:
            params_->dvpp_profiling = cmdInfo.args[opt];
            break;
        case ARGS_POWER:
            params_->low_power = cmdInfo.args[opt];
            break;
        case ARGS_L2_PROFILING:
            params_->l2CacheTaskProfiling = cmdInfo.args[opt];
            break;
        case ARGS_AICPU:
            params_->aicpuTrace = cmdInfo.args[opt];
            break;
        case ARGS_PARSE:
            params_->parseSwitch= cmdInfo.args[opt];
            break;
        case ARGS_QUERY:
            params_->querySwitch= cmdInfo.args[opt];
            break;
        case ARGS_EXPORT:
            params_->exportSwitch= cmdInfo.args[opt];
            break;
        case ARGS_MSPROFTX:
            params_->msproftx = cmdInfo.args[opt];
            break;
        default:
            ParamsSwitchValid3(cmdInfo, opt);
            break;
    }
}

void InputParser::ParamsSwitchValid3(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    switch (opt) {
        case ARGS_BIU:
            params_->biu = cmdInfo.args[opt];
            break;
        default:
            break;
    }
}

void InputParser::ParamsFreqValid(const struct MsprofCmdInfo &cmdInfo, const int freq, int opt)
{
    if (freq > THOUSAND || opt > NR_ARGS) {
        return;
    }
    int interval = THOUSAND / freq;
    switch (opt) {
        case ARGS_CPU_SAMPLING_FREQ:
            params_->cpu_sampling_interval = interval;
            break;
        case ARGS_SYS_SAMPLING_FREQ:
            params_->sys_sampling_interval = interval;
            break;
        case ARGS_PID_SAMPLING_FREQ:
            params_->pid_sampling_interval = interval;
            break;
        case ARGS_IO_SAMPLING_FREQ:
            params_->io_sampling_interval = interval;
            break;
        case ARGS_DVPP_FREQ:
            params_->dvpp_sampling_interval = interval;
            break;
        default:
            break;
    }
    return;
}

int InputParser::MsprofSwitchCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    int ret = CheckArgOnOff(cmdInfo, opt);
    if (ret == MSPROF_DAEMON_OK) {
        ParamsSwitchValid(cmdInfo, opt);
    }

    return ret;
}

int InputParser::MsprofFreqCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    int ret = MSPROF_DAEMON_OK;
    if (opt > NR_ARGS) {
        return MSPROF_DAEMON_ERROR;
    }
    switch (opt) {
        case ARGS_SYS_PERIOD:
            ret = CheckSysPeriodVaild(cmdInfo);
            break;
        case ARGS_SYS_SAMPLING_FREQ:
        case ARGS_PID_SAMPLING_FREQ:
            // 1 - 10
            ret = CheckArgRange(cmdInfo, opt, 1, 10); // 10 : max length
            break;
        case ARGS_CPU_SAMPLING_FREQ:
        case ARGS_INTERCONNECTION_FREQ:
            // 1 - 50
            ret = CheckArgRange(cmdInfo, opt, 1, 50); // 50 : max length
            break;
        case ARGS_IO_SAMPLING_FREQ:
        case ARGS_DVPP_FREQ:
            // 1 - 100
            ret = CheckArgRange(cmdInfo, opt, 1, 100); // 100 : max length
            break;
        case ARGS_HARDWARE_MEM_SAMPLING_FREQ:
            // 1 - 1000
            ret = CheckArgRange(cmdInfo, opt, 1, 1000); // 1000 : max length
            break;
        case ARGS_AIC_FREQ:
        case ARGS_AIV_FREQ:
            // 10 - 1000
            ret = CheckArgRange(cmdInfo, opt, 1, 100); // 10 : min length, 100 : max length
            break;
        case ARGS_BIU_FREQ:
            // 300 - 30000
            ret = CheckArgRange(cmdInfo, opt, BIU_SAMPLE_FREQ_MIN, BIU_SAMPLE_FREQ_MAX);
            break;
        case ARGS_EXPORT_ITERATION_ID:
        case ARGS_EXPORT_MODEL_ID:
            ret = CheckArgsIsNumber(cmdInfo, opt);
            break;
        default:
            break;
    }
    if (ret == MSPROF_DAEMON_OK) {
        MsprofFreqUpdateParams(cmdInfo, opt);
    }
    return ret;
}

void InputParser::MsprofFreqUpdateParams(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    switch (opt) {
        case ARGS_AIC_FREQ:
            params_->aicore_sampling_interval = (THOUSAND / std::stoi(cmdInfo.args[opt]));
            break;
        case ARGS_AIV_FREQ:
            params_->aiv_sampling_interval = (THOUSAND / std::stoi(cmdInfo.args[opt]));
            break;
        case ARGS_BIU_FREQ:
            params_->biu_freq = std::stoi(cmdInfo.args[opt]);
            break;
        case ARGS_SYS_PERIOD:
            params_->profiling_period = std::stoi(cmdInfo.args[opt]);
            break;
        case ARGS_SYS_SAMPLING_FREQ:
        case ARGS_PID_SAMPLING_FREQ:
        case ARGS_IO_SAMPLING_FREQ:
        case ARGS_DVPP_FREQ:
        case ARGS_CPU_SAMPLING_FREQ:
            ParamsFreqValid(cmdInfo, std::stoi(cmdInfo.args[opt]), opt);
            break;
        case ARGS_HARDWARE_MEM_SAMPLING_FREQ:
            params_->hardware_mem_sampling_interval = (THOUSAND / std::stoi(cmdInfo.args[opt]));
            break;
        case ARGS_INTERCONNECTION_FREQ:
            params_->interconnection_sampling_interval = (THOUSAND / std::stoi(cmdInfo.args[opt]));
            break;
        case ARGS_EXPORT_ITERATION_ID:
            params_->exportIterationId = cmdInfo.args[opt];
            break;
        case ARGS_EXPORT_MODEL_ID:
            params_->exportModelId = cmdInfo.args[opt];
            break;
        default:
            break;
    }
}

int InputParser::MsprofCmdCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    int ret = MSPROF_DAEMON_OK;
    if (opt > NR_ARGS) {
        return MSPROF_DAEMON_ERROR;
    }
    switch (opt) {
        case ARGS_OUTPUT:
            ret = CheckOutputValid(cmdInfo);
            break;
        case ARGS_STORAGE_LIMIT:
            ret = CheckStorageLimitValid(cmdInfo);
            break;
        case ARGS_APPLICATION:
            ret = CheckAppValid(cmdInfo);
            break;
        case ARGS_ENVIRONMENT:
            ret = CheckEnvironmentValid(cmdInfo);
            break;
        case ARGS_AIC_MODE:
        case ARGS_AIV_MODE:
            ret = CheckSampleModeValid(cmdInfo, opt);
            break;
        case ARGS_AIC_METRICE:
        case ARGS_AIV_METRICS:
            ret = CheckAiCoreMetricsValid(cmdInfo, opt);
            break;
        case ARGS_SYS_DEVICES:
            ret = CheckSysDevicesVaild(cmdInfo);
            break;
        case ARGS_LLC_PROFILING:
            ret = CheckLlcProfilingVaild(cmdInfo);
            break;
        case ARGS_PYTHON_PATH:
            ret = CheckPythonPathValid(cmdInfo);
            break;
        case ARGS_SUMMARY_FORMAT:
            ret = CheckExportSummaryFormat(cmdInfo);
            break;
        default:
            break;
    }
    return ret;
}

int InputParser::MsprofHostCheckValid(const struct MsprofCmdInfo &cmdInfo, int opt)
{
    int ret = MSPROF_DAEMON_ERROR;
    if (opt > NR_ARGS) {
        return MSPROF_DAEMON_ERROR;
    }
    switch (opt) {
        case ARGS_HOST_SYS:
            ret = CheckHostSysValid(cmdInfo);
            break;
        case ARGS_HOST_SYS_PID:
            ret = CheckHostSysPidValid(cmdInfo);
            break;
        default:
            break;
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
