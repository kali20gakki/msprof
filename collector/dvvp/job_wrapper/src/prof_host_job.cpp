/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: the host job class
 * Author:
 * Create: 2020-02-05
 */

#include "prof_host_job.h"
#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "config/config.h"
#include "logger/msprof_dlog.h"
#include "mmpa_api.h"
#include "platform/platform.h"
#include "uploader_mgr.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::config;
using namespace analysis::dvvp::common::error;
using namespace Analysis::Dvvp::Common::Platform;
using namespace analysis::dvvp::common::utils;
using namespace Analysis::Dvvp::MsprofErrMgr;

const int FILE_FIND_REPLAY          = 10;

static void StartHostProfTimer(TimerHandlerTag tag, SHARED_PTR_ALIA<TimerHandler> handler)
{
    TimerManager::instance()->StartProfTimer();
    TimerManager::instance()->RegisterProfTimerHandler(tag, handler);
}

static void StopHostProfTimer(TimerHandlerTag tag)
{
    TimerManager::instance()->RemoveProfTimerHandler(tag);
    TimerManager::instance()->StopProfTimer();
}

int ProfHostDataBase::CheckHostProfiling(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    if (!(cfg->comParams->params->host_profiling)) {
        return PROFILING_NOTSUPPORT;
    }
    if (Platform::instance()->RunSocSide()) {
        MSPROF_LOGI("Not in host Side, host profiling not enabled");
        return PROFILING_NOTSUPPORT;
    }

#if (defined(_WIN32) || defined(_WIN64) || defined(_MSC_VER))
    MSPROF_LOGI("Currently, only the Linux environment is supported.");
    return PROFILING_NOTSUPPORT;
#endif
    return PROFILING_SUCCESS;
}

int ProfHostDataBase::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    int ret = CheckHostProfiling(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }
    collectionJobCfg_ = cfg;

    analysis::dvvp::transport::UploaderMgr::instance()->GetUploader(cfg->comParams->params->job_id, upLoader_);
    if (upLoader_ == nullptr) {
        MSPROF_LOGE("Failed to get host upLoader, job_id=%s", cfg->comParams->params->job_id.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to get host upLoader, job_id=%s", cfg->comParams->params->job_id.c_str());
        return PROFILING_FAILED;
    }
    // default 20ms collecttion interval
    static const unsigned int profStatIntervalMs = 20; // 20 MS
    static const unsigned int profMsToNs = 1000000; // 1000000 NS
    sampleIntervalNs_ = (static_cast<unsigned long long>(profStatIntervalMs) * profMsToNs);
    return PROFILING_SUCCESS;
}

int ProfHostDataBase::Uninit()
{
    upLoader_.reset();
    return PROFILING_SUCCESS;
}

ProfHostCpuJob::ProfHostCpuJob() : ProfHostDataBase()
{
}

ProfHostCpuJob::~ProfHostCpuJob()
{
}

int ProfHostCpuJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    int ret = ProfHostDataBase::Init(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    if (collectionJobCfg_->comParams->params->host_cpu_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("Host cpu profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfHostCpuJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);

    SHARED_PTR_ALIA<ProcHostCpuHandler> cpuHandler;
    MSVP_MAKE_SHARED7_RET(cpuHandler, ProcHostCpuHandler,
        PROF_HOST_PROC_CPU, PROC_HOST_PROC_DATA_BUF_SIZE, sampleIntervalNs_,
        PROF_HOST_PROC_CPU_USAGE_FILE, collectionJobCfg_->comParams->params,
        collectionJobCfg_->comParams->jobCtx, upLoader_, PROFILING_FAILED);
    if (cpuHandler->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("HostCpuHandler Init Failed");
        MSPROF_INNER_ERROR("EK9999", "HostCpuHandler Init Failed");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("HostCpuHandler Init succ, sampleIntervalNs_:%llu", sampleIntervalNs_);

    StartHostProfTimer(PROF_HOST_PROC_CPU, cpuHandler);
    return PROFILING_SUCCESS;
}

int ProfHostCpuJob::Uninit()
{
    StopHostProfTimer(PROF_HOST_PROC_CPU);
    (void)ProfHostDataBase::Uninit();
    return PROFILING_SUCCESS;
}

ProfHostMemJob::ProfHostMemJob() : ProfHostDataBase()
{
}

ProfHostMemJob::~ProfHostMemJob()
{
}

int ProfHostMemJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    int ret = ProfHostDataBase::Init(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    if (collectionJobCfg_->comParams->params->host_mem_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("Host memory profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfHostMemJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);

    SHARED_PTR_ALIA<ProcHostMemHandler> memHandler;
    MSVP_MAKE_SHARED7_RET(memHandler, ProcHostMemHandler,
        PROF_HOST_PROC_MEM, PROC_HOST_PROC_DATA_BUF_SIZE, sampleIntervalNs_,
        PROF_HOST_PROC_MEM_USAGE_FILE, collectionJobCfg_->comParams->params,
        collectionJobCfg_->comParams->jobCtx, upLoader_, PROFILING_FAILED);
    if (memHandler->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("HostMemHandler Init Failed");
        MSPROF_INNER_ERROR("EK9999", "HostMemHandler Init Failed");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("HostMemHandler Init succ, sampleIntervalNs_:%llu", sampleIntervalNs_);

    StartHostProfTimer(PROF_HOST_PROC_MEM, memHandler);
    return PROFILING_SUCCESS;
}

int ProfHostMemJob::Uninit()
{
    StopHostProfTimer(PROF_HOST_PROC_MEM);
    (void)ProfHostDataBase::Uninit();
    return PROFILING_SUCCESS;
}

ProfHostNetworkJob::ProfHostNetworkJob() : ProfHostDataBase()
{
}

ProfHostNetworkJob::~ProfHostNetworkJob()
{
}

int ProfHostNetworkJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    int ret = ProfHostDataBase::Init(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    if (collectionJobCfg_->comParams->params->host_network_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("Host network profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfHostNetworkJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);

    SHARED_PTR_ALIA<ProcHostNetworkHandler> networkHandler;
    MSVP_MAKE_SHARED7_RET(networkHandler, ProcHostNetworkHandler,
        PROF_HOST_SYS_NETWORK, PROC_HOST_PROC_DATA_BUF_SIZE, sampleIntervalNs_,
        PROF_HOST_SYS_NETWORK_USAGE_FILE, collectionJobCfg_->comParams->params,
        collectionJobCfg_->comParams->jobCtx, upLoader_, PROFILING_FAILED);
    if (networkHandler->Init() != PROFILING_SUCCESS) {
        MSPROF_LOGE("HostNetworkHandler Init Failed");
        MSPROF_INNER_ERROR("EK9999", "HostNetworkHandler Init Failed");
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("HostNetworkHandler Init succ, sampleIntervalNs_:%llu", sampleIntervalNs_);

    StartHostProfTimer(PROF_HOST_SYS_NETWORK, networkHandler);
    return PROFILING_SUCCESS;
}

int ProfHostNetworkJob::Uninit()
{
    StopHostProfTimer(PROF_HOST_SYS_NETWORK);
    (void)ProfHostDataBase::Uninit();
    return PROFILING_SUCCESS;
}

ProfHostSysCallsJob::ProfHostSysCallsJob() : ProfHostDataBase()
{
}

ProfHostSysCallsJob::~ProfHostSysCallsJob()
{
}

int ProfHostSysCallsJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    int ret = ProfHostDataBase::CheckHostProfiling(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->host_osrt_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("Host_syscalls_profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfHostSysCallsJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(profHostService_, ProfHostService, PROFILING_FAILED);

    int ret = profHostService_->Init(collectionJobCfg_, PROF_HOST_SYS_CALL);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[ProfHostSysCallsJob]Failed Init profHostService_");
        MSPROF_INNER_ERROR("EK9999", "Failed Init profHostService_");
        return ret;
    }

    ret = profHostService_->Start();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[ProfHostSysCallsJob]Failed Start profHostService_");
        MSPROF_INNER_ERROR("EK9999", "Failed Start profHostService_");
        return ret;
    }
    return ret;
}

int ProfHostSysCallsJob::Uninit()
{
    MSPROF_LOGI("Start ProfHostSysCallsJob Uninit");
    if (profHostService_ == nullptr) {
        MSPROF_LOGE("ProfHostSysCallsJob profHostService_ is null");
        MSPROF_INNER_ERROR("EK9999", "ProfHostSysCallsJob profHostService_ is null");
        return PROFILING_FAILED;
    }
    profHostService_->WakeupTimeoutEnd();
    int ret = profHostService_->Stop();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[HostSysCalls]Failed Stop profHostService_");
        MSPROF_INNER_ERROR("EK9999", "[HostSysCalls]Failed Stop profHostService_");
    }
    return ret;
}

ProfHostPthreadJob::ProfHostPthreadJob() : ProfHostDataBase()
{
}

ProfHostPthreadJob::~ProfHostPthreadJob()
{
}

int ProfHostPthreadJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    int ret = ProfHostDataBase::CheckHostProfiling(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->host_osrt_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("Host_pthread_profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfHostPthreadJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(profHostService_, ProfHostService, PROFILING_FAILED);

    int ret = profHostService_->Init(collectionJobCfg_, PROF_HOST_SYS_PTHREAD);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[ProfHostPthreadJob]Failed Init profHostService_");
        MSPROF_INNER_ERROR("EK9999", "Failed Init profHostService_");
        return ret;
    }

    ret = profHostService_->Start();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[ProfHostPthreadJob]Failed Start profHostService_");
        MSPROF_INNER_ERROR("EK9999", "Failed Start profHostService_");
        return ret;
    }
    return ret;
}

int ProfHostPthreadJob::Uninit()
{
    MSPROF_LOGI("Start ProfHostPthreadJob Uninit");
    if (profHostService_ == nullptr) {
        MSPROF_LOGE("ProfHostPthreadJob profHostService_ is null");
        MSPROF_INNER_ERROR("EK9999", "ProfHostPthreadJob profHostService_ is null");
        return PROFILING_FAILED;
    }
    profHostService_->WakeupTimeoutEnd();
    int ret = profHostService_->Stop();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[HostPthread]Failed Stop profHostService_");
        MSPROF_INNER_ERROR("EK9999", "[HostPthread]Failed Stop profHostService_");
    }
    return ret;
}

ProfHostDiskJob::ProfHostDiskJob() : ProfHostDataBase()
{
}

ProfHostDiskJob::~ProfHostDiskJob()
{
}

int ProfHostDiskJob::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    int ret = ProfHostDataBase::CheckHostProfiling(cfg);
    if (ret != PROFILING_SUCCESS) {
        return ret;
    }

    collectionJobCfg_ = cfg;
    if (collectionJobCfg_->comParams->params->host_disk_profiling.compare(
        analysis::dvvp::common::config::MSVP_PROF_ON) != 0) {
        MSPROF_LOGI("Host_disk_profiling not enabled");
        return PROFILING_FAILED;
    }
    return PROFILING_SUCCESS;
}

int ProfHostDiskJob::Process()
{
    CHECK_JOB_COMMON_PARAM_RET(collectionJobCfg_, PROFILING_FAILED);
    MSVP_MAKE_SHARED0_RET(profHostService_, ProfHostService, PROFILING_FAILED);

    int ret = profHostService_->Init(collectionJobCfg_, PROF_HOST_SYS_DISK);
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[ProfHostDiskJob]Failed Init profHostService_");
        MSPROF_INNER_ERROR("EK9999", "Failed Init profHostService_");
        return ret;
    }

    ret = profHostService_->Start();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[ProfHostDiskJob]Failed Start profHostService_");
        MSPROF_INNER_ERROR("EK9999", "Failed Start profHostService_");
        return ret;
    }
    return ret;
}

int ProfHostDiskJob::Uninit()
{
    MSPROF_LOGI("Start ProfHostDiskJob Uninit");
    if (profHostService_ == nullptr) {
        MSPROF_LOGE("ProfHostDiskJob profHostService_ is null");
        MSPROF_INNER_ERROR("EK9999", "ProfHostDiskJob profHostService_ is null");
        return PROFILING_FAILED;
    }
    profHostService_->WakeupTimeoutEnd();
    int ret = profHostService_->Stop();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("[HostDisk]Failed Stop profHostService_");
        MSPROF_INNER_ERROR("EK9999", "[HostDisk]Failed Stop profHostService_");
    }
    return ret;
}

ProfHostService::ProfHostService() : hostTimerTag_(PROF_HOST_MAX_TAG), hostProcess_(MSVP_MMPROCESS)
{
}

ProfHostService::~ProfHostService()
{
}

int ProfHostService::GetCmdStr(int hostSysPid, std::string &profHostCmd)
{
    int ret = PROFILING_FAILED;
    switch (hostTimerTag_) {
        case PROF_HOST_SYS_CALL:
            ret = GetCollectSysCallsCmd(hostSysPid, profHostCmd);
            break;
        case PROF_HOST_SYS_PTHREAD:
            ret = GetCollectPthreadsCmd(hostSysPid, profHostCmd);
            break;
        case PROF_HOST_SYS_DISK:
            ret = GetCollectIOTopCmd(hostSysPid, profHostCmd);
            break;
        default:
            break;
    }
    return ret;
}

/**
 * Start the collection tool process.
 */
int ProfHostService::Process()
{
    std::string profHostCmd;
    int hostSysPid = collectionJobCfg_->comParams->params->host_sys_pid;
    int ret = GetCmdStr(hostSysPid, profHostCmd);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Get profHostCmd failed,toolName:%s", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "Get profHostCmd failed,toolName:%s", toolName_.c_str());
        return PROFILING_FAILED;
    }
    if (profHostCmd.size() > 0) {
        MSPROF_LOGI("Begin to start profiling:%s,cmd=%s", toolName_.c_str(), profHostCmd.c_str());
        std::vector<std::string> params = analysis::dvvp::common::utils::Utils::Split(profHostCmd.c_str());
        if (params.empty()) {
            MSPROF_LOGE("profHostCmd:%s empty", toolName_.c_str());
            MSPROF_INNER_ERROR("EK9999", "profHostCmd:%s empty", toolName_.c_str());
            return PROFILING_FAILED;
        }
        std::string cmd = params[0];
        std::vector<std::string> argsV;
        std::vector<std::string> envsV;
        if (params.size() > 1) {
            argsV.assign(params.begin() + 1, params.end());
        }
        envsV.push_back("PATH=/usr/bin/:/usr/sbin:/var");
        hostProcess_ = MSVP_MMPROCESS;
        int exitCode = analysis::dvvp::common::utils::INVALID_EXIT_CODE;
        std::string redirectionPath = profHostOutDir_ + std::to_string(outDataNumber_);
        hostWriteDoneInfo_.startTime =
                    std::to_string(analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw());
        ExecCmdParams execCmdParams(cmd, true, redirectionPath);
        ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envsV, exitCode, hostProcess_);
        MSPROF_LOGI("start profiling hostjob %s, pid = %u, ret=%d", toolName_.c_str(), (unsigned int)hostProcess_, ret);
        FUNRET_CHECK_FAIL_PRINT(ret, PROFILING_SUCCESS);
    }
    ret = WaitCollectToolStart();
    return ret;
}

int ProfHostService::KillToolAndWaitHostProcess() const
{
    static const std::string ENV_PATH = "PATH=/usr/bin:/usr/sbin";
    std::vector<std::string> envV;
    envV.push_back(ENV_PATH);
    std::vector<std::string> argsV;
    argsV.push_back(PROF_SCRIPT_FILE_PATH);
    argsV.push_back("pkill");
    argsV.push_back(toolName_);
    int exitCode = analysis::dvvp::common::utils::VALID_EXIT_CODE;
    static const std::string CMD = "sudo";
    mmProcess appProcess = MSVP_MMPROCESS;
    ExecCmdParams execCmdParams(CMD, false, "");
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envV, exitCode, appProcess);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to kill process %s, ret=%d, exitCode=%d", toolName_.c_str(), ret, exitCode);
        MSPROF_INNER_ERROR("EK9999", "Failed to kill process %s, ret=%d, exitCode=%d",
            toolName_.c_str(), ret, exitCode);
        return ret;
    }
    if (hostProcess_ > 0) {
        bool isExited = false;
        ret = analysis::dvvp::common::utils::Utils::WaitProcess(hostProcess_, isExited, exitCode, true);
        if (ret != PROFILING_SUCCESS) {
            MSPROF_LOGE("Failed to wait process %d, ret=%d,  exitCode=%d",
                reinterpret_cast<int>(hostProcess_), ret, exitCode);
            MSPROF_INNER_ERROR("EK9999", "Failed to wait process %d, ret=%d, exitCode=%d",
                reinterpret_cast<int>(hostProcess_), ret, exitCode);
            return ret;
        } else {
            MSPROF_LOGI("Process %d exited, exitcode=%d", reinterpret_cast<int>(hostProcess_), exitCode);
        }
    }
    return PROFILING_SUCCESS;
}

/**
 *  Stop collection tool process.
 */
int ProfHostService::Uninit()
{
    int ret = KillToolAndWaitHostProcess();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGI("Failed to kill process %s, ", toolName_.c_str());
        return ret;
    }
    MSPROF_LOGI("Succeeded to kill process %s", toolName_.c_str());
    if (hostTimerTag_ == PROF_HOST_SYS_DISK) {
        std::string fileName = profHostOutDir_ + std::to_string(outDataNumber_);
        std::string diskIoStartTime = "start_time " + hostWriteDoneInfo_.startTime + "\n";
        std::ofstream in;
        in.open(fileName, std::ios::app);
        if (!in.is_open()) {
            MSPROF_LOGE("Failed to open %s", fileName.c_str());
            MSPROF_INNER_ERROR("EK9999", "Failed to open %s", fileName.c_str());
            return PROFILING_FAILED;
        }
        in << diskIoStartTime;
        in.close();
    }
    ret = WaitCollectToolEnd();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGW("Failed to kill process %s", toolName_.c_str());
    }
    ret = WriteDone();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("ProfHostService, Failed to WriteDone, toolName:%s", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "ProfHostService, Failed to WriteDone, toolName:%s", toolName_.c_str());
    }
    return ret;
}

int ProfHostService::GetCollectSysCallsCmd(int pid, std::string &profHostCmd)
{
    if (pid < 0) {
        MSPROF_LOGE("ProfHostSysCallsJob pid: %d is not vaild", pid);
        MSPROF_INNER_ERROR("EK9999", "ProfHostSysCallsJob pid: %d is not vaild", pid);
        return PROFILING_FAILED;
    }
    std::string profHostOutDir = profHostOutDir_ + std::to_string(outDataNumber_);
    std::stringstream ssPerfHostCmd;
    ssPerfHostCmd << "sudo ";
    ssPerfHostCmd << PROF_SCRIPT_FILE_PATH;
    ssPerfHostCmd << " perf ";
    ssPerfHostCmd << pid;

    profHostCmd = ssPerfHostCmd.str();
    startProcessCmd_ = "perf trace -T --syscalls";
    return PROFILING_SUCCESS;
}

int ProfHostService::GetCollectPthreadsCmd(int pid, std::string &profHostCmd)
{
    if (pid < 0) {
        MSPROF_LOGE("ProfHostPthreadJob pid: %d is not vaild", pid);
        MSPROF_INNER_ERROR("EK9999", "ProfHostPthreadJob pid: %d is not vaild", pid);
        return PROFILING_FAILED;
    }
    std::string profHostOutDir = profHostOutDir_ + std::to_string(outDataNumber_);
    std::stringstream ssPerfHostCmd;
    ssPerfHostCmd << "sudo ";
    ssPerfHostCmd << PROF_SCRIPT_FILE_PATH;
    ssPerfHostCmd << " ltrace ";
    ssPerfHostCmd << pid;

    profHostCmd = ssPerfHostCmd.str();
    startProcessCmd_ = "ltrace -ttt -T -e pthread_";
    return PROFILING_SUCCESS;
}

int ProfHostService::GetCollectIOTopCmd(int pid, std::string &profHostCmd)
{
    if (pid < 0) {
        MSPROF_LOGE("ProfHostDiskJob pid: %d is not vaild", pid);
        MSPROF_INNER_ERROR("EK9999", "ProfHostDiskJob pid: %d is not vaild", pid);
        return PROFILING_FAILED;
    }
    std::stringstream ssPerfHostCmd;
    ssPerfHostCmd << "sudo ";
    ssPerfHostCmd << PROF_SCRIPT_FILE_PATH;
    ssPerfHostCmd << " iotop ";
    ssPerfHostCmd << pid;

    profHostCmd = ssPerfHostCmd.str();
    startProcessCmd_ = "iotop -b -d";
    return PROFILING_SUCCESS;
}

int ProfHostService::Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg, const HostTimerHandlerTag hostTimerTag)
{
    CHECK_JOB_CONTEXT_PARAM_RET(cfg, PROFILING_FAILED);
    CHECK_TIMER_TAG_PARAM_RET(hostTimerTag, PROFILING_FAILED);

    MSVP_MAKE_SHARED0_RET(collectionJobCfg_, CollectionJobCfg, PROFILING_FAILED);
    collectionJobCfg_ = cfg;
    hostTimerTag_ = hostTimerTag;
    toolName_ = PROF_HOST_TOOL_NAME[hostTimerTag_];
    profHostOutDir_ = collectionJobCfg_->comParams->tmpResultDir + PROF_HOST_OUTDATA_PATH[hostTimerTag_];
    isStarted_ = true;
    return PROFILING_SUCCESS;
}

int ProfHostService::Start()
{
    MSPROF_LOGI("Start ProfHostService begin, toolName:%s", toolName_.c_str());
    if (!isStarted_) {
        MSPROF_LOGE("ProfHostService not started, toolName:%s", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "ProfHostService not started, toolName:%s", toolName_.c_str());
        return PROFILING_FAILED;
    }
    std::string threadName = "MSVP_PORF_HOST_TOOL_TIMER";
    analysis::dvvp::common::thread::Thread::SetThreadName(threadName);
    int ret = analysis::dvvp::common::thread::Thread::Start();
    if (ret != PROFILING_SUCCESS) {
        isStarted_ = false;
        MSPROF_LOGE("Thread start failed, toolName:%s", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "Thread start failed, toolName:%s", toolName_.c_str());
        return PROFILING_FAILED;
    }

    MSPROF_LOGI("Start ProfHostService success, toolName:%s", toolName_.c_str());
    return PROFILING_SUCCESS;
}

int ProfHostService::Stop()
{
    MSPROF_LOGI("Stop ProfHostService begin, toolName:%s", toolName_.c_str());
    if (!isStarted_) {
        MSPROF_LOGE("ProfHostService not started, toolName:%s", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "ProfHostService not started, toolName:%s", toolName_.c_str());
        return PROFILING_FAILED;
    }
    isStarted_ = false;

    int ret = analysis::dvvp::common::thread::Thread::Stop();
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Thread stop failed, toolName:%s", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "Thread stop failed, toolName:%s", toolName_.c_str());
        return PROFILING_FAILED;
    }
    MSPROF_LOGI("Stop ProfHostService success, toolName:%s", toolName_.c_str());

    return PROFILING_SUCCESS;
}

void ProfHostService::Run(const struct error_message::Context &errorContext)
{
    MsprofErrorManager::instance()->SetErrorContext(errorContext);
    int ret = Process();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("The run toolName:%s process failed.", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "The run toolName:%s process failed.", toolName_.c_str());
        return;
    }
    do {
        std::string fileName = profHostOutDir_ + std::to_string(outDataNumber_);
        long long len = analysis::dvvp::common::utils::Utils::GetFileSize(fileName);
        if (len > MSVP_SMALL_FILE_MAX_LEN) {
            Handler();
            MSPROF_LOGI("The file:%s data is too large, and the file is fragmented.",
                        fileName.c_str());
        }
        WaitTimeoutEnd();
    } while (isStarted_);
    Uninit();
}

/**
 *  When the data volume is too large,
 *  stop the current collection process and then start a new collection process.
 */
int ProfHostService::Handler()
{
    MSPROF_LOGI("Run Handler start");
    int ret = Uninit();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("The toolName:%s unint failed.", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "The toolName:%s unint failed.", toolName_.c_str());
        return ret;
    }
    outDataNumber_++;
    ret = Process();
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("The toolName:%s process failed.", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "The toolName:%s process failed.", toolName_.c_str());
    }
    return ret;
}

int ProfHostService::WriteDone()
{
    std::string fileName = profHostOutDir_ + std::to_string(outDataNumber_);
    long long len = analysis::dvvp::common::utils::Utils::GetFileSize(fileName);
    if (len < 0) {
        MSPROF_LOGE("ProfHostService GetFileSize failed");
        MSPROF_INNER_ERROR("EK9999", "ProfHostService GetFileSize failed");
        return PROFILING_FAILED;
    }
    std::string startTime = "starttime:" + hostWriteDoneInfo_.startTime + "\n";
    hostWriteDoneInfo_.fileSize = "filesize:" + std::to_string(len) + "\n";
    hostWriteDoneInfo_.endTime = "endtime  :"
                                + std::to_string(analysis::dvvp::common::utils::Utils::GetClockMonotonicRaw())
                                + "\n";
    fileName += ".done";
    std::ofstream in;
    in.open(fileName, std::ios::trunc);
    if (!in.is_open()) {
        MSPROF_LOGE("Failed to open %s", fileName.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to open %s", fileName.c_str());
        return PROFILING_FAILED;
    }
    in << hostWriteDoneInfo_.fileSize << startTime << hostWriteDoneInfo_.endTime;
    in.close();
    MSPROF_LOGI("Succeeded to write done data");
    return PROFILING_SUCCESS;
}

void ProfHostService::PrintFileContent(const std::string filePath) const
{
    std::string context;
    std::ifstream psFile;
    psFile.open(filePath, std::ifstream::in);
    if (psFile.is_open()) {
        while (getline(psFile, context)) {
            MSPROF_LOGD("File:%s, context:%s", filePath.c_str(), context.c_str());
        }
        psFile.close();
    } else {
        MSPROF_LOGE("File:%s, open failed", filePath.c_str());
    }
}

int ProfHostService::CollectToolIsRun()
{
    int hostSysPid = collectionJobCfg_->comParams->params->host_sys_pid;
    std::string checkToolRunCmd = "ps -ef | grep \"" + startProcessCmd_ + "\" | grep -v grep | grep "
                    + std::to_string(hostSysPid);
    std::vector<std::string> envV;
    envV.push_back("PATH=/usr/bin:/usr/sbin");
    std::vector<std::string> argsV;
    argsV.push_back("-c");
    argsV.push_back(checkToolRunCmd);
    int exitCode = VALID_EXIT_CODE;
    mmProcess appProcess = MSVP_MMPROCESS;
    std::string redirectionPath = profHostOutDir_ + "temp" + std::to_string(Utils::GetClockRealtime());
    ExecCmdParams execCmdParams("sh", false, redirectionPath);
    int ret = analysis::dvvp::common::utils::Utils::ExecCmd(execCmdParams, argsV, envV, exitCode, appProcess);
    if (ret != PROFILING_SUCCESS) {
        MSPROF_LOGE("Failed to check process %s, ret=%d", toolName_.c_str(), ret);
        MSPROF_INNER_ERROR("EK9999", "Failed to check process %s, ret=%d", toolName_.c_str(), ret);
        return ret;
    }
    for (int i = 0; i < FILE_FIND_REPLAY; ++i) {
        if (!(Utils::IsFileExist(redirectionPath))) {
            mmSleep(1); // If the file is not found, the delay is 1 ms.
            continue;
        } else {
            break;
        }
    }
    if (!(Utils::IsFileExist(redirectionPath))) {
        MSPROF_LOGE("The file:%s is not exist", redirectionPath.c_str());
        MSPROF_INNER_ERROR("EK9999", "The file:%s is not exist", redirectionPath.c_str());
        return PROFILING_FAILED;
    }
    PrintFileContent(redirectionPath);
    long long len = Utils::GetFileSize(redirectionPath);
    mmUnlink(redirectionPath.c_str());
    if (len > 0) {
        return PROFILING_SUCCESS;
    }
    MSPROF_LOGI("The tool:%s is not running", toolName_.c_str());
    return PROFILING_FAILED;
}

int ProfHostService::WaitCollectToolStart()
{
    int ret = PROFILING_FAILED;
    for (int i = 0; i < FILE_FIND_REPLAY; ++i) {
        ret = CollectToolIsRun();
        if (ret != PROFILING_SUCCESS) {
            continue;
        } else {
            break;
        }
    }
    if (ret == PROFILING_FAILED) {
        MSPROF_LOGE("Failed to start the process: %s", toolName_.c_str());
        MSPROF_INNER_ERROR("EK9999", "Failed to start the process: %s", toolName_.c_str());
    }
    return ret;
}

int ProfHostService::WaitCollectToolEnd()
{
    int ret = PROFILING_FAILED;
    for (int i = 0; i < FILE_FIND_REPLAY; ++i) {
        ret = CollectToolIsRun();
        if (ret == PROFILING_SUCCESS) {
            continue;
        } else {
            break;
        }
    }
    if (ret == PROFILING_SUCCESS) {
        MSPROF_LOGW("Failed to end the process: %s", toolName_.c_str());
        ret = PROFILING_FAILED;
    } else {
        ret = PROFILING_SUCCESS;
    }
    return ret;
}

void ProfHostService::WakeupTimeoutEnd()
{
    std::lock_guard<std::mutex> lk(needUnintMtx_);
    isJobUnint_.notify_all();
}

void ProfHostService::WaitTimeoutEnd()
{
    MSPROF_LOGI("Wakeup Unint start");
    std::unique_lock<std::mutex> lk(needUnintMtx_);
    static const int CHECK_FILE_SIZE_INTERVAL_US = 500000; // 500000 means 500 ms
    auto res = isJobUnint_.wait_for(lk, std::chrono::microseconds(CHECK_FILE_SIZE_INTERVAL_US));
    if (res == std::cv_status::timeout) {
        MSPROF_LOGI("Wakeup Unint timeout");
        return;
    }
}
}}}
