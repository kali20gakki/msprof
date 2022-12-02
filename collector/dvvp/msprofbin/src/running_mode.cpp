/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: msprof bin prof task
 * Author: fsx
 * Create: 2021-10-26
 */

#include "running_mode.h"
#include "errno/error_code.h"
#include "input_parser.h"
#include "cmd_log.h"
#include "msprof_dlog.h"
#include "ai_drv_dev_api.h"
#include "prof_params_adapter.h"
#include "config/config_manager.h"
#include "application.h"
#include "transport/file_transport.h"
#include "transport/uploader_mgr.h"
#include "task_relationship_mgr.h"
#include "platform/platform.h"
#include "env_manager.h"
#include "mmpa_api.h"

namespace Collector {
namespace Dvvp {
namespace Msprofbin {
using namespace analysis::dvvp::message;
using namespace analysis::dvvp::common::error;
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::driver;
using namespace Analysis::Dvvp::Msprof;
using namespace Analysis::Dvvp::Common::Config;
using namespace Analysis::Dvvp::Common::Platform;
using namespace analysis::dvvp::common::utils;
using namespace Collector::Dvvp::Plugin;
using namespace Collector::Dvvp::Mmpa;

RunningMode::RunningMode(std::string preCheckParams, std::string modeName, SHARED_PTR_ALIA<ProfileParams> params)
    : isQuit_(false), modeName_(modeName), taskPid_(MSVP_MMPROCESS), preCheckParams_(preCheckParams),
      blackSet_(), whiteSet_(), neccessarySet_(), params_(params), taskMap_()
{}

RunningMode::~RunningMode() {}

std::string RunningMode::ConvertParamsSetToString(std::set<int>& srcSet) const
{
    std::string result;
    if (srcSet.empty()) {
        return result;
    }
    std::stringstream ssParams;
    for (int paramId: srcSet) {
        ssParams << "--" << longOptions[paramId].name << " ";
    }
    result = ssParams.str();
    Utils::RemoveEndCharacter(result, ' ');
    return result;
}

int RunningMode::CheckForbiddenParams() const
{
    if (params_ == nullptr || params_->usedParams.empty()) {
        return PROFILING_FAILED;
    }
    std::set<int> usedForbiddenParams;
    set_intersection(params_->usedParams.begin(), params_->usedParams.end(),
        blackSet_.begin(), blackSet_.end(), inserter(usedForbiddenParams, usedForbiddenParams.begin()));
    if (!usedForbiddenParams.empty()) {
        std::string forbiddenParamsStr = ConvertParamsSetToString(usedForbiddenParams);
        CmdLog::instance()->CmdErrorLog("The argument %s is forbidden when --%s is not empty",
            forbiddenParamsStr.c_str(), preCheckParams_.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int RunningMode::CheckNeccessaryParams() const
{
    if (params_ == nullptr || params_->usedParams.empty()) {
        return PROFILING_FAILED;
    }
    std::set<int> moreReqParams;
    set_difference(neccessarySet_.begin(), neccessarySet_.end(),
        params_->usedParams.begin(), params_->usedParams.end(),
        inserter(moreReqParams, moreReqParams.begin()));
    if (!moreReqParams.empty()) {
        std::string reqParams = ConvertParamsSetToString(moreReqParams);
        CmdLog::instance()->CmdErrorLog("The argument %s is neccessary when --%s is not empty",
            reqParams.c_str(), preCheckParams_.c_str());
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

void RunningMode::OutputUselessParams() const
{
    if (params_ == nullptr || params_->usedParams.empty()) {
        return;
    }
    std::set<int> uselessParams;
    set_difference(params_->usedParams.begin(), params_->usedParams.end(),
        whiteSet_.begin(), whiteSet_.end(),
        inserter(uselessParams, uselessParams.begin()));
    if (!uselessParams.empty()) {
        std::string noEffectParams = ConvertParamsSetToString(uselessParams);
        CmdLog::instance()->CmdWarningLog("The argument %s is useless when --%s is not empty",
            noEffectParams.c_str(), preCheckParams_.c_str());
    }
}

void RunningMode::UpdateOutputDirInfo()
{
    int ret = GetOutputDirInfoFromRecord();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("Get output info failed.");
    }
}

int RunningMode::GetOutputDirInfoFromParams()
{
    if (!jobResultDir_.empty()) {
        MSPROF_LOGW("Get output info from params failed, exist output info before.");
        return PROFILING_FAILED;
    }
    jobResultDir_ = params_->result_dir;
    return PROFILING_SUCCESS;
}

int RunningMode::GetOutputDirInfoFromRecord()
{
    if (!jobResultDir_.empty()) {
        MSPROF_LOGW("Get output info from record failed, exist output info before.");
        return PROFILING_FAILED;
    }
    std::string baseDir;
    std::ifstream fin;
    std::string resultDirTmp = params_->result_dir;
    Utils::EnsureEndsInSlash(resultDirTmp);
    std::string pidStr = std::to_string(Utils::GetPid());
    std::string recordFile = pidStr + MSVP_UNDERLINE + OUTPUT_RECORD;
    std::string outPut = resultDirTmp + recordFile;
    long long fileSize = Utils::GetFileSize(outPut);
    if (fileSize > MSVP_SMALL_FILE_MAX_LEN || fileSize <= 0) {
        MSPROF_LOGE("File size is invalid. fileName:%s, size:%lld.", recordFile.c_str(), fileSize);
        return PROFILING_FAILED;
    }
    fin.open(outPut, std::ifstream::in);
    if (fin.is_open()) {
        while (getline(fin, baseDir)) {
            jobResultDirList_.insert(resultDirTmp + baseDir);
        }
        fin.close();
        RemoveRecordFile(outPut);
        return PROFILING_SUCCESS;
    } else {
        char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
        MSPROF_LOGE("Open file failed, fileName:%s, error: %s", Utils::BaseName(recordFile).c_str(),
            MmGetErrorFormatMessage(MmGetErrorCode(),
                errBuf, MAX_ERR_STRING_LEN));
        return PROFILING_FAILED;
    }
}

void RunningMode::RemoveRecordFile(const std::string &fileName) const
{
    if (!(Utils::IsFileExist(fileName))) {
        return;
    }
    if (remove(fileName.c_str()) != EOK) {
        MSPROF_LOGE("Remove file: %s failed!", Utils::BaseName(fileName).c_str());
        return;
    }
    return;
}

int RunningMode::HandleProfilingParams() const
{
    if (params_ == nullptr) {
        MSPROF_LOGE("ProfileParams is not valid!");
        return PROFILING_FAILED;
    }
    if (params_->devices.compare("all") == 0) {
        params_->devices = DrvGetDevIdsStr();
    }
    std::string aicoreMetricsType =
        params_->ai_core_metrics.empty() ? PIPE_UTILIZATION : params_->ai_core_metrics;
    int ret = ConfigManager::instance()->GetAicoreEvents(aicoreMetricsType, params_->ai_core_profiling_events);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("The ai_core_metrics is invalid");
        return PROFILING_FAILED;
    }
    params_->ai_core_metrics = aicoreMetricsType;
    std::string aivMetricsType =
        params_->aiv_metrics.empty() ? PIPE_UTILIZATION : params_->aiv_metrics;
    ret = ConfigManager::instance()->GetAicoreEvents(aivMetricsType, params_->aiv_profiling_events);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("The aiv_metrics (%s) is invalid.", aivMetricsType.c_str());
        return PROFILING_FAILED;
    }
    params_->aiv_metrics = aivMetricsType;
    ConfigManager::instance()->MsprofL2CacheAdapter(params_);
    Analysis::Dvvp::Msprof::ProfParamsAdapter::instance()->GenerateLlcEvents(params_);
    params_->msprofBinPid = Utils::GetPid();
    return ProfParamsAdapter::instance()->UpdateParams(params_);
}

void RunningMode::StopRunningTasks() const
{
    if (taskPid_ == MSVP_MMPROCESS) {
        return;
    }
    static const std::string ENV_PATH = "PATH=/usr/bin/:/usr/sbin:/var:/bin";
    std::vector<std::string> envsV;
    envsV.push_back(ENV_PATH);
    std::string cmd = "kill";
    ExecCmdParams execCmdParams(cmd, true, "");
    std::vector<std::string> argsV;
    argsV.push_back(std::to_string(reinterpret_cast<int>(taskPid_)));
    int exitCode = INVALID_EXIT_CODE;
    MmProcess killProces = MSVP_MMPROCESS;
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envsV, exitCode, killProces);
    MSPROF_LOGI("[%s mode] Stop %s Process: %d, ret=%d", modeName_.c_str(), taskName_.c_str(),
        reinterpret_cast<int>(taskPid_), ret);
}

void RunningMode::SetEnvList(std::vector<std::string> &envsV)
{
    envsV = Analysis::Dvvp::App::EnvManager::instance()->GetGlobalEnv();
}

int RunningMode::CheckCurrentDataPathValid(const std::string &path) const
{
    static const std::string infoJson = "info.json";
    std::vector<std::string> fileList;
    Utils::GetChildFilenames(path, fileList);
    std::string filePath;
    std::vector<std::string> softLinkList;
    bool isInfoJsonExist = false;
    for (auto file : fileList) {
        filePath = path + MSVP_SLASH + file;
        if (Utils::IsSoftLink(filePath)) {
            softLinkList.push_back(filePath);
            continue;
        }
        if (!Utils::IsDir(filePath) && file.substr(0, infoJson.size()) == infoJson) {
            isInfoJsonExist = true;
        }
    }
    if (softLinkList.size()) {
        for (auto softLink : softLinkList) {
            MSPROF_LOGE("The path '%s' is soft link, not support.", filePath.c_str());
        }
        CmdLog::instance()->CmdErrorLog("Argument --output is invalid because of soft link file");
        return PROFILING_INVALID_PARAM;
    }
    return isInfoJsonExist == true ? PROFILING_SUCCESS : PROFILING_FAILED;
}

int RunningMode::CheckParseDataPathValid(const std::string &parseDataPath) const
{
    std::vector<std::string> fileList;
    Utils::GetChildFilenames(parseDataPath, fileList);
    std::string filePath;
    int ret = PROFILING_FAILED;
    for (auto file : fileList) {
        filePath = parseDataPath + MSVP_SLASH + file;
        if (!Utils::IsDir(filePath)) {
            continue;
        }
        ret = CheckCurrentDataPathValid(filePath);
        if (ret == PROFILING_INVALID_PARAM || ret == PROFILING_SUCCESS) {
            return ret;
        }
    }
    return PROFILING_FAILED;
}

ParseDataType RunningMode::GetParseDataType(const std::string &parseDataPath) const
{
    int ret;
    // cmd : msprof --parse=on --output=PROF_xxx/device_x/
    ret = CheckCurrentDataPathValid(parseDataPath);
    if (ret == PROFILING_INVALID_PARAM) {
        return ParseDataType::DATA_PATH_INVALID;
    } else if (ret == PROFILING_SUCCESS) {
        return ParseDataType::DATA_PATH_NON_CLUSTER;
    }
    // cmd : msprof --parse=on --output=PROF_xxx/
    ret = CheckParseDataPathValid(parseDataPath);
    if (ret == PROFILING_INVALID_PARAM) {
        return ParseDataType::DATA_PATH_INVALID;
    } else if (ret == PROFILING_SUCCESS) {
        return ParseDataType::DATA_PATH_NON_CLUSTER;
    }

    // cmd : msprof --parse=on --output=cluster_data/
    std::vector<std::string> currentDirFileList;
    Utils::GetChildFilenames(parseDataPath, currentDirFileList);
    std::string currentDirFilePath;
    for (auto currentDirFile : currentDirFileList) {
        currentDirFilePath = parseDataPath + MSVP_SLASH + currentDirFile;
        if (!Utils::IsDir(currentDirFilePath)) {
            continue;
        }
        ret = CheckParseDataPathValid(currentDirFilePath);
        if (ret == PROFILING_INVALID_PARAM) {
            return ParseDataType::DATA_PATH_INVALID;
        } else if (ret == PROFILING_SUCCESS) {
            return ParseDataType::DATA_PATH_CLUSTER;
        }
    }
    return ParseDataType::DATA_PATH_INVALID;
}

int RunningMode::StartParseTask()
{
    if (isQuit_) {
        MSPROF_LOGE("Start parse task error, msprofbin has quited");
        return PROFILING_FAILED;
    }
    if (taskPid_ != MSVP_MMPROCESS) {
        MSPROF_LOGE("Start parse task error, process %d is running,"
            "only one child process can run at the same time.", reinterpret_cast<int>(taskPid_));
        return PROFILING_FAILED;
    }
    if (jobResultDir_.empty() || analysisPath_.empty()) {
        MSPROF_LOGE("Start parse task error, get output dir and msprof.py script failed.");
        return PROFILING_FAILED;
    }
    CmdLog::instance()->CmdInfoLog("Start parse data in %s.", Utils::BaseName(jobResultDir_).c_str());
    taskName_="Parse";

    std::string cmd = params_->pythonPath;
    ExecCmdParams execCmdParams(cmd, true, "");
    std::vector<std::string> envsV;
    int exitCode = INVALID_EXIT_CODE;
    SetEnvList(envsV);
    ParseDataType DataType = GetParseDataType(jobResultDir_);
    if (DataType == ParseDataType::DATA_PATH_INVALID) {
        MSPROF_LOGE("[parse mode]Parse invalid data in path %s.", Utils::BaseName(jobResultDir_).c_str());
        return PROFILING_FAILED;
    }
    std::vector<std::string> argsV;
    argsV.push_back(analysisPath_);
    argsV.push_back("import");
    if (DataType == ParseDataType::DATA_PATH_CLUSTER) {
        argsV.push_back("--cluster");
    }
    argsV.push_back("-dir=" + jobResultDir_);
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envsV, exitCode, taskPid_);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Failed to launch parse task, data path: %s", Utils::BaseName(jobResultDir_).c_str());
        return PROFILING_FAILED;
    }
    ret = WaitRunningProcess(taskName_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to wait parse process %d to exit, ret=%d", reinterpret_cast<int>(taskPid_), ret);
        return PROFILING_FAILED;
    }
    taskPid_ = MSVP_MMPROCESS;
    exitCode = INVALID_EXIT_CODE;
    CmdLog::instance()->CmdInfoLog("Parse all data in %s done.", Utils::BaseName(jobResultDir_).c_str());
    return PROFILING_SUCCESS;
}

int RunningMode::StartQueryTask()
{
    if (isQuit_) {
        MSPROF_LOGE("Start query task error, msprofbin has quited");
        return PROFILING_FAILED;
    }
    if (taskPid_ != MSVP_MMPROCESS) {
        MSPROF_LOGE("Start query task error, process %d is running,"
            "only one child process can run at the same time.", reinterpret_cast<int>(taskPid_));
        return PROFILING_FAILED;
    }
    if (jobResultDir_.empty() || analysisPath_.empty()) {
        MSPROF_LOGE("Start query task error, get output dir and msprof.py script failed.");
        return PROFILING_FAILED;
    }
    CmdLog::instance()->CmdInfoLog("Start query data in %s.", Utils::BaseName(jobResultDir_).c_str());
    taskName_="Query";

    std::string cmd = params_->pythonPath;
    ExecCmdParams execCmdParams(cmd, true, "");
    std::vector<std::string> envsV;
    int exitCode = INVALID_EXIT_CODE;
    SetEnvList(envsV);
    std::vector<std::string> argsV = {
        analysisPath_,
        "query",
        "-dir=" + jobResultDir_
    };
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envsV, exitCode, taskPid_);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Failed to launch query task, data path: %s", Utils::BaseName(jobResultDir_).c_str());
        return PROFILING_FAILED;
    }
    ret = WaitRunningProcess(taskName_);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to wait query process %d to exit, ret=%d", reinterpret_cast<int>(taskPid_), ret);
        return PROFILING_FAILED;
    }
    taskPid_ = MSVP_MMPROCESS;
    exitCode = INVALID_EXIT_CODE;
    CmdLog::instance()->CmdInfoLog("Query all data in %s done.", Utils::BaseName(jobResultDir_).c_str());
    return PROFILING_SUCCESS;
}

int RunningMode::RunExportSummaryTask(const ExecCmdParams &execCmdParams,
    std::vector<std::string> &envsV, int &exitCode)
{
    std::vector<std::string> argsSummaryV = {
        analysisPath_,
        "export", "summary",
        "-dir=" + jobResultDir_,
        "--iteration-id=" + params_->exportIterationId,
        "--format=" + params_->exportSummaryFormat
    };
    if (params_->exportModelId != DEFAULT_MODEL_ID) {
        argsSummaryV.emplace_back("--model-id=" + params_->exportModelId);
    }
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsSummaryV, envsV, exitCode, taskPid_);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Failed to launch export summary task, data path: %s", Utils::BaseName(jobResultDir_).c_str());
        return PROFILING_FAILED;
    }
    ret = WaitRunningProcess(taskName_ + " summary");
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to wait export summary process %d to exit, ret=%d", reinterpret_cast<int>(taskPid_), ret);
        return PROFILING_FAILED;
    }
    taskPid_ = MSVP_MMPROCESS;
    exitCode = INVALID_EXIT_CODE;
    return PROFILING_SUCCESS;
}
int RunningMode::RunExportTimelineTask(const ExecCmdParams &execCmdParams,
    std::vector<std::string> &envsV, int &exitCode)
{
    std::vector<std::string> argsTimelineV = {
        analysisPath_,
        "export", "timeline",
        "-dir=" + jobResultDir_,
        "--iteration-id=" + params_->exportIterationId
    };
    if (params_->exportModelId != DEFAULT_MODEL_ID) {
        argsTimelineV.emplace_back("--model-id=" + params_->exportModelId);
    }
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsTimelineV, envsV, exitCode, taskPid_);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Failed to launch export timeline task, data path: %s", Utils::BaseName(jobResultDir_).c_str());
        return PROFILING_FAILED;
    }
    ret = WaitRunningProcess(taskName_ + " timeline");
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to wait export timeline process %d to exit, ret=%d", reinterpret_cast<int>(taskPid_), ret);
        return PROFILING_FAILED;
    }
    taskPid_ = MSVP_MMPROCESS;
    exitCode = INVALID_EXIT_CODE;
    return PROFILING_SUCCESS;
}

int RunningMode::StartExportTask()
{
    if (isQuit_) {
        MSPROF_LOGE("Start export task error, msprofbin has quited");
        return PROFILING_FAILED;
    }
    if (taskPid_ != MSVP_MMPROCESS) {
        MSPROF_LOGE("Start export task error, process %d is running,"
            "only one child process can run at the same time.", reinterpret_cast<int>(taskPid_));
        return PROFILING_FAILED;
    }
    if (jobResultDir_.empty() || analysisPath_.empty()) {
        MSPROF_LOGE("Start export task error, get output dir and msprof.py script failed.");
        return PROFILING_FAILED;
    }
    CmdLog::instance()->CmdInfoLog("Start export data in %s.", Utils::BaseName(jobResultDir_).c_str());
    taskName_="Export";

    std::string cmd = params_->pythonPath;
    ExecCmdParams execCmdParams(cmd, true, "");
    std::vector<std::string> envsV;
    int exitCode = INVALID_EXIT_CODE;
    SetEnvList(envsV);
    if (RunExportSummaryTask(execCmdParams, envsV, exitCode) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Export summary failed in %s.", Utils::BaseName(jobResultDir_).c_str());
        return PROFILING_FAILED;
    }
    if (RunExportTimelineTask(execCmdParams, envsV, exitCode) != PROFILING_SUCCESS) {
        MSPROF_LOGE("Export timeline failed in %s.", Utils::BaseName(jobResultDir_).c_str());
        return PROFILING_FAILED;
    }
    CmdLog::instance()->CmdInfoLog("Export all data in %s done.", Utils::BaseName(jobResultDir_).c_str());
    return PROFILING_SUCCESS;
}

int RunningMode::WaitRunningProcess(std::string processUsage) const
{
    if (taskPid_ == MSVP_MMPROCESS) {
        MSPROF_LOGE("[WaitRunningProcess] Invalid task pid.");
        return PROFILING_FAILED;
    }
    bool isExited = false;
    int exitCode = 0;
    int ret = PROFILING_SUCCESS;
    static const int sleepIntevalUs = 1000000;

    for (;;) {
        ret = analysis::dvvp::common::utils::Utils::WaitProcess(taskPid_, isExited, exitCode, false);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to wait %s process %d to exit, ret=%d", processUsage.c_str(),
                reinterpret_cast<int>(taskPid_), ret);
            return ret;
        }
        if (isExited) {
            MSPROF_EVENT("%s process %d exited, exit code:%d", processUsage.c_str(),
                reinterpret_cast<int>(taskPid_), exitCode);
            return PROFILING_SUCCESS;
        }
        analysis::dvvp::common::utils::Utils::UsleepInterupt(sleepIntevalUs);
    }
}

SHARED_PTR_ALIA<ProfTask> RunningMode::GetRunningTask(const std::string &jobId)
{
    auto iter = taskMap_.find(jobId);
    if (iter != taskMap_.end()) {
        return iter->second;
    } else {
        return nullptr;
    }
}

int RunningMode::CheckAnalysisEnv()
{
    if (isQuit_) {
        MSPROF_LOGE("Check Analysis env failed, msprofbin has quited");
        return PROFILING_FAILED;
    }
    if (params_->pythonPath.empty() && Utils::PythonEnvReady()) {
        const std::string PYTHON_CMD{"python3"};
        params_->pythonPath = PYTHON_CMD;
    }
    if (!Utils::AnalysisEnvReady(analysisPath_) || analysisPath_.empty()) {
        CmdLog::instance()->CmdWarningLog("Get msprof.py path failed.");
        return PROFILING_FAILED;
    }
    if (!Utils::IsFileExist(analysisPath_)) {
        CmdLog::instance()->CmdWarningLog("The msprof.py file is not found, so analysis is not supported.");
        return PROFILING_FAILED;
    }
    if (MmAccess2(analysisPath_, M_X_OK) != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdWarningLog("Analysis script permission denied, path: %s",
            Utils::BaseName(analysisPath_).c_str());
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Found avaliable analysis script, script path: %s", Utils::BaseName(analysisPath_).c_str());

    return PROFILING_SUCCESS;
}

AppMode::AppMode(std::string preCheckParams, SHARED_PTR_ALIA<ProfileParams> params)
    : RunningMode(preCheckParams, "app", params)
{
    whiteSet_ = {
        ARGS_OUTPUT, ARGS_STORAGE_LIMIT, ARGS_APPLICATION, ARGS_ENVIRONMENT, ARGS_AIC_MODE,
        ARGS_AIC_METRICE, ARGS_AIV_MODE, ARGS_AIV_METRICS, ARGS_LLC_PROFILING,
        ARGS_ASCENDCL, ARGS_AI_CORE, ARGS_AIV, ARGS_MODEL_EXECUTION, ARGS_L2_SAMPLE_FREQ,
        ARGS_RUNTIME_API, ARGS_TASK_TIME, ARGS_AICPU, ARGS_CPU_PROFILING, ARGS_SYS_PROFILING,
        ARGS_PID_PROFILING, ARGS_HARDWARE_MEM, ARGS_IO_PROFILING, ARGS_INTERCONNECTION_PROFILING,
        ARGS_DVPP_PROFILING, ARGS_L2_PROFILING, ARGS_AIC_FREQ, ARGS_AIV_FREQ, ARGS_BIU_FREQ, ARGS_BIU, ARGS_HCCL,
        ARGS_SYS_SAMPLING_FREQ, ARGS_PID_SAMPLING_FREQ, ARGS_HARDWARE_MEM_SAMPLING_FREQ,
        ARGS_IO_SAMPLING_FREQ, ARGS_DVPP_FREQ,  ARGS_CPU_SAMPLING_FREQ, ARGS_INTERCONNECTION_FREQ,
        ARGS_HOST_SYS, ARGS_HOST_SYS_USAGE, ARGS_HOST_SYS_USAGE_FREQ, ARGS_PYTHON_PATH, ARGS_MSPROFTX
    };
}

AppMode::~AppMode() {}

int AppMode::ModeParamsCheck()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[App Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    OutputUselessParams();
    return PROFILING_SUCCESS;
}

int AppMode::RunModeTasks()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[App Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    if (StartAppTask() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[App Mode] Run application task failed.");
        return PROFILING_FAILED;
    }
    UpdateOutputDirInfo();

    if (jobResultDirList_.empty()) {
        MSPROF_LOGE("[App Mode] Invalid collection result.");
        return PROFILING_FAILED;
    }

    if (CheckAnalysisEnv() != PROFILING_SUCCESS) {
        MSPROF_LOGW("[App Mode] Analysis environment is not OK, auto parse will not start.");
        return PROFILING_SUCCESS;
    }

    std::set<std::string>::iterator iter;
    for (iter = jobResultDirList_.begin(); iter != jobResultDirList_.end(); ++iter) {
        jobResultDir_ = *iter;
        if (StartExportTask() != PROFILING_SUCCESS) {
            MSPROF_LOGW("[App Mode] Run export task failed.");
            return PROFILING_SUCCESS;
        }
        if (StartQueryTask() != PROFILING_SUCCESS) {
            MSPROF_LOGW("[App Mode] Run query task failed.");
            return PROFILING_SUCCESS;
        }
    }
    return PROFILING_SUCCESS;
}

int AppMode::StartAppTask(bool needWait)
{
    if (isQuit_) {
        MSPROF_LOGE("Failed to launch app, msprofbin has quited");
        return PROFILING_FAILED;
    }
    int ret = analysis::dvvp::app::Application::LaunchApp(params_, taskPid_);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Failed to launch app");
        return PROFILING_FAILED;
    }
    taskName_="app";
    if (needWait) {
        // wait app exit
        ret = WaitRunningProcess("App");
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to wait process %d to exit, ret=%d", reinterpret_cast<int>(taskPid_), ret);
            return PROFILING_FAILED;
        }
    }
    taskPid_ = MSVP_MMPROCESS;
    return PROFILING_SUCCESS;
}

SystemMode::SystemMode(std::string preCheckParams, SHARED_PTR_ALIA<ProfileParams> params)
    : RunningMode(preCheckParams, "system", params)
{
    whiteSet_ = {
        ARGS_OUTPUT, ARGS_STORAGE_LIMIT, ARGS_AIC_MODE, ARGS_SYS_DEVICES,
        ARGS_AIC_METRICE, ARGS_AIV_MODE, ARGS_AIV_METRICS, ARGS_LLC_PROFILING,
        ARGS_AI_CORE, ARGS_AIV, ARGS_CPU_PROFILING, ARGS_SYS_PROFILING, ARGS_L2_SAMPLE_FREQ,
        ARGS_PID_PROFILING, ARGS_HARDWARE_MEM, ARGS_IO_PROFILING, ARGS_INTERCONNECTION_PROFILING,
        ARGS_DVPP_PROFILING, ARGS_L2_PROFILING, ARGS_AIC_FREQ, ARGS_AIV_FREQ, ARGS_BIU_FREQ, ARGS_BIU,
        ARGS_SYS_SAMPLING_FREQ, ARGS_PID_SAMPLING_FREQ, ARGS_HARDWARE_MEM_SAMPLING_FREQ,
        ARGS_IO_SAMPLING_FREQ, ARGS_DVPP_FREQ,  ARGS_CPU_SAMPLING_FREQ, ARGS_INTERCONNECTION_FREQ,
        ARGS_HOST_SYS, ARGS_HOST_SYS_USAGE, ARGS_HOST_SYS_USAGE_FREQ, ARGS_SYS_PERIOD,
        ARGS_HOST_SYS_PID, ARGS_PYTHON_PATH
    };
    neccessarySet_ = {ARGS_OUTPUT, ARGS_SYS_PERIOD};
}

SystemMode::~SystemMode() {}

int SystemMode::CheckIfDeviceOnline() const
{
    if (params_->devices.compare("all") == 0) {
        return PROFILING_SUCCESS;
    }
    auto devIds = Utils::Split(params_->devices, false, "", ",");
    int numDevices = DrvGetDevNum();
    if (numDevices <= 0) {
        CmdLog::instance()->CmdErrorLog("Get dev's num %d failed", numDevices);
        return PROFILING_FAILED;
    }
    std::vector<int> devices;
    int ret = DrvGetDevIds(numDevices, devices);
    if (ret != PROFILING_SUCCESS) {
        CmdLog::instance()->CmdErrorLog("Get dev's id failed");
        return PROFILING_FAILED;
    }
    UtilsStringBuilder<int> builder;
    builder.Join(devices, ",");
    std::set<std::string> validIds;
    std::vector<std::string> invalidIds;
    for (auto devId : devIds) {
        if (!Utils::CheckStringIsNonNegativeIntNum(devId)) {
            CmdLog::instance()->CmdErrorLog("The device (%s) is not valid, please check it!", devId.c_str());
            return PROFILING_FAILED;
        }
        auto it = find(devices.begin(), devices.end(), std::stoi(devId));
        if (it == devices.end()) {
            invalidIds.push_back(devId);
        } else {
            validIds.insert(devId);
        }
    }
    UtilsStringBuilder<std::string> strBuilder;
    if (!invalidIds.empty()) {
        CmdLog::instance()->CmdWarningLog("The following devices(%s) are offline and will not collect.",
            strBuilder.Join(invalidIds, ",").c_str());
    }
    if (validIds.empty()) {
        CmdLog::instance()->CmdErrorLog("Argument --sys-devices is invalid,"
            "Please enter a valid --sys-devices value.");
        return PROFILING_FAILED;
    }
    std::vector<std::string> validIdList;
    validIdList.assign(validIds.begin(), validIds.end());
    params_->devices = strBuilder.Join(validIdList, ",");
    return PROFILING_SUCCESS;
}

int SystemMode::CheckHostSysParams() const
{
    if (params_->host_sys.empty() && params_->host_sys_usage.empty() && CheckIfDeviceOnline() != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    if (!params_->host_sys.empty() && params_->host_sys_pid < 0) {
        CmdLog::instance()->CmdErrorLog("The argument --host-sys-pid"
            " are required when --application is empty and --host-sys is on.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

bool SystemMode::DataWillBeCollected() const
{
    if (params_ == nullptr || params_->usedParams.empty()) {
        return false;
    }
    std::set<int> unneccessaryParams;
    set_difference(params_->usedParams.begin(), params_->usedParams.end(),
        neccessarySet_.begin(), neccessarySet_.end(),
        inserter(unneccessaryParams, unneccessaryParams.begin()));
    set_intersection(params_->usedParams.begin(), params_->usedParams.end(),
        unneccessaryParams.begin(), unneccessaryParams.end(),
        inserter(unneccessaryParams, unneccessaryParams.begin()));
    if (unneccessaryParams.size() == 1 && *(unneccessaryParams.begin()) == ARGS_SYS_DEVICES) {
        CmdLog::instance()->CmdWarningLog("No collection data type is specified, profiling will not start");
        return false;
    }
    return true;
}

int SystemMode::ModeParamsCheck()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[System Mode] Invalid params!");
        return PROFILING_FAILED;
    }

    if (CheckNeccessaryParams() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[System Mode] Check neccessary params failed!");
        return PROFILING_FAILED;
    }
    if (!DataWillBeCollected()) {
        MSPROF_LOGE("[System Mode] No data will be collected!");
        return PROFILING_FAILED;
    }
    OutputUselessParams();

    if (CheckHostSysParams() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[System Mode] Check host sys params failed!");
        return PROFILING_FAILED;
    }

    if (HandleProfilingParams() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[System Mode] HandleProfilingParams failed");
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int SystemMode::RunModeTasks()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[System Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    if (StartSysTask() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[System Mode] Run system task failed.");
        return PROFILING_FAILED;
    }
    UpdateOutputDirInfo();
    if (CheckAnalysisEnv() != PROFILING_SUCCESS) {
        MSPROF_LOGW("[System Mode] Analysis environment is not OK, auto parse will not start.");
        return PROFILING_SUCCESS;
    }

    std::set<std::string>::iterator iter;
    for (iter = jobResultDirList_.begin(); iter != jobResultDirList_.end(); ++iter) {
        jobResultDir_ = *iter;
        if (StartExportTask() != PROFILING_SUCCESS) {
            MSPROF_LOGW("[System Mode] Run export task failed.");
            return PROFILING_SUCCESS;
        }
        if (StartQueryTask() != PROFILING_SUCCESS) {
            MSPROF_LOGW("[System Mode] Run query task failed.");
            return PROFILING_SUCCESS;
        }
    }
    return PROFILING_SUCCESS;
}

int SystemMode::CreateJobDir(std::string device, std::string &resultDir) const
{
    resultDir = params_->result_dir;
    resultDir = resultDir + MSVP_SLASH + baseDir_ + MSVP_SLASH;
    resultDir.append(Utils::CreateResultPath(device));
    if (Utils::CreateDir(resultDir) != PROFILING_SUCCESS) {
        char errBuf[MAX_ERR_STRING_LEN + 1] = {0};
        CmdLog::instance()->CmdErrorLog("Create dir (%s) failed.ErrorCode: %d, ErrorInfo: %s.",
            Utils::BaseName(resultDir).c_str(),
            MmGetErrorCode(),
            MmGetErrorFormatMessage(MmGetErrorCode(),
                errBuf, MAX_ERR_STRING_LEN));
        return PROFILING_FAILED;
    }
    analysis::dvvp::common::utils::Utils::EnsureEndsInSlash(resultDir);
    return PROFILING_SUCCESS;
}

int SystemMode::RecordOutPut() const
{
    std::string pidStr = std::to_string(Utils::GetPid());
    std::string recordFile = pidStr + MSVP_UNDERLINE + OUTPUT_RECORD;
    std::string absolutePath = params_->result_dir + MSVP_SLASH + recordFile;
    std::ofstream file;
    file.open(absolutePath, std::ios::out | std::ios::app);
    if (!file.is_open()) {
        MSPROF_LOGE("Failed to open %s", Utils::BaseName(recordFile).c_str());
        return PROFILING_FAILED;
    }
    file << baseDir_ << std::endl << std::flush;
    file.close();

    return PROFILING_SUCCESS;
}

int SystemMode::StartHostTask(const std::string resultDir, const std::string device)
{
    auto params = GenerateHostParam(params_);
    params->devices = device;
    if (std::stoi(device) == DEFAULT_HOST_ID) {
        params->host_profiling = TRUE;
    }
    bool retu = CreateSampleJsonFile(params, resultDir);
    if (!retu) {
        MSPROF_LOGE("CreateSampleJsonFile failed");
        return PROFILING_FAILED;
    }
    params->result_dir = resultDir;
    int ret = CreateUploader(params->job_id, resultDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("CreateUploader failed");
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<ProfSocTask> task = nullptr;
    MSVP_MAKE_SHARED2_RET(task, ProfSocTask, std::stoi(device), params, PROFILING_FAILED);
    ret = task->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("DeviceTask init failed, deviceId:%s", device.c_str());
        return PROFILING_FAILED;
    }
    ret = task->Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("DeviceTask start failed, deviceId:%s", device.c_str());
        return PROFILING_FAILED;
    }
    taskMap_[params->job_id] = task;
    return PROFILING_SUCCESS;
}

int SystemMode::StartDeviceTask(const std::string resultDir, const std::string device)
{
    auto params = GenerateDeviceParam(params_);
    params->devices = device;
    int ret = CreateUploader(params->job_id, resultDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("CreateUploader failed");
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<ProfRpcTask> task = nullptr;
    MSVP_MAKE_SHARED2_RET(task, ProfRpcTask, std::stoi(device), params, PROFILING_FAILED);
    ret = task->Init();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[StartDeviceTask]DeviceTask init failed, deviceId:%s", device.c_str());
        return PROFILING_FAILED;
    }
    ret = task->Start();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("[StartDeviceTask]DeviceTask start failed, deviceId:%s", device.c_str());
        return PROFILING_FAILED;
    }
    taskMap_[params->job_id] = task;
    return PROFILING_SUCCESS;
}

void SystemMode::StopTask()
{
    for (auto task : taskMap_) {
        task.second->Stop();
        task.second->Wait();
    }
    taskMap_.clear();
}

int SystemMode::WaitSysTask() const
{
    unsigned int times = 0;
    unsigned int cycles = 0;

    if (params_->profiling_period > 0) {
        cycles = (unsigned int)(params_->profiling_period * US_TO_SECOND_TIMES(PROCESS_WAIT_TIME));
    }
    while ((times < cycles) && (!isQuit_)) {
        analysis::dvvp::common::utils::Utils::UsleepInterupt(PROCESS_WAIT_TIME);
        times++;
    }
    return PROFILING_SUCCESS;
}

bool SystemMode::IsDeviceJob() const
{
    if (params_ == nullptr) {
        return false;
    }
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE &&
        params_->hardware_mem.compare("on") == 0) {
        return true;
    }
    if (params_->cpu_profiling.compare("on") == 0 || params_->sys_profiling.compare("on") == 0
        || params_->pid_profiling.compare("on") == 0) {
        return true;
    } else {
        return false;
    }
}

int SystemMode::CreateUploader(const std::string &jobId, const std::string &resultDir) const
{
    if (jobId.empty() || resultDir.empty()) {
        return PROFILING_FAILED;
    }
    auto transport = analysis::dvvp::transport::FileTransportFactory().CreateFileTransport(
        resultDir, params_->storageLimit, true);
    if (transport == nullptr) {
        MSPROF_LOGE("CreateFileTransport failed, key:%s", jobId.c_str());
        return PROFILING_FAILED;
    }
    int ret = analysis::dvvp::transport::UploaderMgr::instance()->CreateUploader(jobId, transport);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("CreateUploader failed, key:%s", jobId.c_str());
        return PROFILING_FAILED;
    }
    Analysis::Dvvp::TaskHandle::TaskRelationshipMgr::instance()->AddLocalFlushJobId(jobId);

    return PROFILING_SUCCESS;
}

SHARED_PTR_ALIA<ProfileParams> SystemMode::GenerateHostParam(
    SHARED_PTR_ALIA<ProfileParams> params)
{
    if (params == nullptr) {
        return nullptr;
    }
    SHARED_PTR_ALIA<ProfileParams> dstParams = nullptr;
    MSVP_MAKE_SHARED0_RET(dstParams, ProfileParams, nullptr);
    std::string dstParamsStr = params->ToString();
    if (!(dstParams->FromString(dstParamsStr))) {
        MSPROF_LOGE("[ProfManager::StartTask]Failed to parse dstParamsStr.");
        return nullptr;
    }
    uintptr_t addr = reinterpret_cast<uintptr_t>(dstParams.get());
    dstParams->job_id = Utils::ProfCreateId((uint64_t)addr);
    if (dstParams->ai_core_profiling_mode.empty()) {
        dstParams->ai_core_profiling_mode = PROFILING_MODE_SAMPLE_BASED;
    }
    if (dstParams->aiv_profiling_mode.empty()) {
        dstParams->aiv_profiling_mode = PROFILING_MODE_SAMPLE_BASED;
    }
    dstParams->msprof_llc_profiling = params->msprof_llc_profiling;
    dstParams->host_sys_pid = params->host_sys_pid;
    dstParams->profiling_period = params->profiling_period;
    return dstParams;
}

SHARED_PTR_ALIA<ProfileParams> SystemMode::GenerateDeviceParam(
    SHARED_PTR_ALIA<ProfileParams> params) const
{
    if (params == nullptr) {
        return nullptr;
    }
    SHARED_PTR_ALIA<ProfileParams> dstParams = nullptr;
    MSVP_MAKE_SHARED0_RET(dstParams, ProfileParams, nullptr);
    dstParams->result_dir = params->result_dir;
    dstParams->sys_profiling = params->sys_profiling;
    dstParams->sys_sampling_interval = params->sys_sampling_interval;
    dstParams->pid_profiling = params->pid_profiling;
    dstParams->pid_sampling_interval = params->pid_sampling_interval;
    dstParams->cpu_profiling = params->cpu_profiling;
    dstParams->cpu_sampling_interval = params->cpu_sampling_interval;
    dstParams->aiCtrlCpuProfiling = params->aiCtrlCpuProfiling;
    dstParams->ai_ctrl_cpu_profiling_events = params->ai_ctrl_cpu_profiling_events;
    uintptr_t addr = reinterpret_cast<uintptr_t>(dstParams.get());
    dstParams->job_id = Utils::ProfCreateId((uint64_t)addr);
    if (ConfigManager::instance()->GetPlatformType() == PlatformType::MINI_TYPE) {
        dstParams->llc_profiling_events = params->llc_profiling_events;
        dstParams->llc_profiling = params->llc_profiling;
        dstParams->msprof_llc_profiling = params->msprof_llc_profiling;
    }
    return dstParams;
}

bool SystemMode::CreateSampleJsonFile(SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params,
    const std::string &resultDir)
{
    if (resultDir.empty()) {
        return true;
    }
    static const std::string fileName = "sample.json";
    int ret = Utils::CreateDir(resultDir);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("create dir error , %s", Utils::BaseName(resultDir).c_str());
        Utils::PrintSysErrorMsg();
        return false;
    }
    MSPROF_LOGI("CreateSampleJsonFile ");
    ret = WriteCtrlDataToFile(resultDir + fileName, params->ToString().c_str(), params->ToString().size());
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to write local files");
        return false;
    }

    return true;
}

bool SystemMode::CreateDoneFile(const std::string &absolutePath, const std::string &fileSize) const
{
    std::ofstream file;

    file.open(absolutePath, std::ios::out);
    if (!file.is_open()) {
        MSPROF_LOGE("Failed to open %s", Utils::BaseName(absolutePath) .c_str());
        return false;
    }
    file << "filesize: " << fileSize << std::endl;
    file.flush();
    file.close();

    return true;
}

int SystemMode::WriteCtrlDataToFile(const std::string &absolutePath, const std::string &data, int dataLen) const
{
    std::ofstream file;

    if (Utils::IsFileExist(absolutePath)) {
        MSPROF_LOGI("file exist: %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_SUCCESS;
    }

    if (data.empty() || dataLen <= 0) {
        MSPROF_LOGE("Failed to open %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_FAILED;
    }
    file.open(absolutePath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        MSPROF_LOGE("Failed to open %s", Utils::BaseName(absolutePath).c_str());
        return PROFILING_FAILED;
    }
    file.write(data.c_str(), dataLen);
    file.flush();
    file.close();

    if (!(CreateDoneFile(absolutePath + ".done", std::to_string(dataLen)))) {
        MSPROF_LOGE("set device done file failed");
        return PROFILING_FAILED;
    }

    return PROFILING_SUCCESS;
}

int SystemMode::StartDeviceJobs(const std::string& device)
{
    std::string resultDir;
    int ret = CreateJobDir(device, resultDir);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    // this host data is for device mapping(llc, ddr, aicore, etc)
    ret = StartHostTask(resultDir, device);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("StartHostTask failed");
        StopTask();
        return PROFILING_FAILED;
    }
    if (IsDeviceJob() && (!Platform::instance()->PlatformIsSocSide())) {
        ret = StartDeviceTask(resultDir, device);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("StartDeviceTask failed");
            StopTask();
            return PROFILING_FAILED;
        }
    }
    return PROFILING_SUCCESS;
}

int SystemMode::StartHostJobs()
{
    std::string hostId = std::to_string(DEFAULT_HOST_ID);
    std::string resultDir;
    int ret = CreateJobDir(hostId, resultDir);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    ret = StartHostTask(resultDir, hostId);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("StartHostTask failed");
        StopTask();
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int SystemMode::StartSysTask()
{
    std::vector<std::string> devices = Utils::Split(params_->devices, false, "", ",");
    params_->PrintAllFields();
    baseDir_ = Utils::CreateTaskId(0);
    if (RecordOutPut() != PROFILING_SUCCESS) {
        MSPROF_LOGW("Failed to record output dir: %s", Utils::BaseName(baseDir_).c_str());
    }
    for (auto device : devices) {
        if (StartDeviceJobs(device) != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    if (params_->IsHostProfiling()) {
        if (StartHostJobs() != PROFILING_SUCCESS) {
            return PROFILING_FAILED;
        }
    }
    int waitRet = WaitSysTask();
    if (waitRet != PROFILING_SUCCESS) {
        MSPROF_LOGE("WaitSysTask failed");
        return PROFILING_FAILED;
    }
    StopTask();
    return waitRet;
}

ParseMode::ParseMode(std::string preCheckParams, SHARED_PTR_ALIA<ProfileParams> params)
    : RunningMode(preCheckParams, "parse", params)
{
    whiteSet_ = {ARGS_OUTPUT, ARGS_PARSE, ARGS_PYTHON_PATH};
    neccessarySet_ = {ARGS_OUTPUT, ARGS_PARSE};
    blackSet_ = {ARGS_QUERY, ARGS_EXPORT};
}

ParseMode::~ParseMode() {}

int ParseMode::ModeParamsCheck()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[Parse Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    if (CheckForbiddenParams() != PROFILING_SUCCESS ||
        CheckNeccessaryParams() != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    OutputUselessParams();

    return PROFILING_SUCCESS;
}

void ParseMode::UpdateOutputDirInfo()
{
    int ret = GetOutputDirInfoFromParams();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("Get output info failed.");
    }
}

int ParseMode::RunModeTasks()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[Parse Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    if (CheckAnalysisEnv() != PROFILING_SUCCESS) {
        MSPROF_LOGW("[Parse Mode] Analysis environment is not OK, parse will not start.");
        return PROFILING_FAILED;
    }
    UpdateOutputDirInfo();
    if (StartParseTask() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Parse Mode] Run parse task failed.");
        return PROFILING_FAILED;
    }
    if (StartQueryTask() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Parse Mode] Run query task failed.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

QueryMode::QueryMode(std::string preCheckParams, SHARED_PTR_ALIA<ProfileParams> params)
    : RunningMode(preCheckParams, "query", params)
{
    whiteSet_ = {ARGS_OUTPUT, ARGS_QUERY, ARGS_PYTHON_PATH};
    neccessarySet_ = {ARGS_OUTPUT, ARGS_QUERY};
    blackSet_ = {ARGS_PARSE, ARGS_EXPORT};
}

QueryMode::~QueryMode() {}

int QueryMode::ModeParamsCheck()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[Query Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    if (CheckForbiddenParams() != PROFILING_SUCCESS ||
        CheckNeccessaryParams() != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    OutputUselessParams();

    return PROFILING_SUCCESS;
}

void QueryMode::UpdateOutputDirInfo()
{
    int ret = GetOutputDirInfoFromParams();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("Get output info failed.");
    }
}

int QueryMode::RunModeTasks()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[Query Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    UpdateOutputDirInfo();
    if (CheckAnalysisEnv() != PROFILING_SUCCESS) {
        MSPROF_LOGW("[Query Mode] Analysis environment is not OK, query will not start.");
        return PROFILING_FAILED;
    }
    if (StartQueryTask() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Query Mode] Run query task failed.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

ExportMode::ExportMode(std::string preCheckParams, SHARED_PTR_ALIA<ProfileParams> params)
    : RunningMode(preCheckParams, "export", params)
{
    whiteSet_ = {
        ARGS_OUTPUT, ARGS_EXPORT, ARGS_EXPORT_ITERATION_ID,
        ARGS_EXPORT_MODEL_ID, ARGS_SUMMARY_FORMAT, ARGS_PYTHON_PATH
    };
    neccessarySet_ = {ARGS_OUTPUT, ARGS_EXPORT};
    blackSet_ = {ARGS_QUERY, ARGS_PARSE};
}

ExportMode::~ExportMode() {}

int ExportMode::ModeParamsCheck()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[Export Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    if (CheckForbiddenParams() != PROFILING_SUCCESS ||
        CheckNeccessaryParams() != PROFILING_SUCCESS) {
        return PROFILING_FAILED;
    }
    OutputUselessParams();

    return PROFILING_SUCCESS;
}

void ExportMode::UpdateOutputDirInfo()
{
    int ret = GetOutputDirInfoFromParams();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("Get output info failed.");
    }
}

int ExportMode::RunModeTasks()
{
    if (params_ == nullptr) {
        MSPROF_LOGE("[Export Mode] Invalid params!");
        return PROFILING_FAILED;
    }
    UpdateOutputDirInfo();
    if (CheckAnalysisEnv() != PROFILING_SUCCESS) {
        MSPROF_LOGW("[Export Mode] Analysis environment is not OK, export will not start.");
        return PROFILING_FAILED;
    }
    if (StartExportTask() != PROFILING_SUCCESS) {
        MSPROF_LOGE("[Export Mode] Run export task failed.");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}
}
}
}