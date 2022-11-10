/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: class timer
 * Author:
 * Create: 2020-02-05
 */
#ifndef ANALYSIS_DVVP_JOB_WRAPPER_PROF_HOST_JOB_H
#define ANALYSIS_DVVP_JOB_WRAPPER_PROF_HOST_JOB_H

#include "prof_timer.h"
#include "collection_register.h"
#include "prof_job.h"
#include "mmpa_api.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
enum HostTimerHandlerTag {
    PROF_HOST_SYS_CALL = 0,
    PROF_HOST_SYS_PTHREAD,
    PROF_HOST_SYS_DISK,
    PROF_HOST_MAX_TAG
};

static const std::string PROF_HOST_TOOL_NAME[PROF_HOST_MAX_TAG] = {
    "perf",
    "ltrace",
    "iotop"
};

static const std::string PROF_HOST_OUTDATA_PATH[PROF_HOST_MAX_TAG] = {
    "data/host_syscall.data.slice_",
    "data/host_pthreadcall.data.slice_",
    "data/host_disk.data.slice_"
};

struct ProfHostWriteDoneInfo {
    std::string fileSize;
    std::string startTime;
    std::string endTime;
};

static const unsigned int PROC_HOST_PROC_DATA_BUF_SIZE = (1 << 13); // 1 << 13  means 8k

// task interface
class ProfHostDataBase : public ICollectionJob {
public:
    ProfHostDataBase() : sampleIntervalNs_(0)
    {
    }
    ~ProfHostDataBase() override {}
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Uninit() override;
    int CheckHostProfiling(const SHARED_PTR_ALIA<CollectionJobCfg> cfg);

protected:
    SHARED_PTR_ALIA<CollectionJobCfg> collectionJobCfg_;
    SHARED_PTR_ALIA<analysis::dvvp::transport::Uploader> upLoader_;
    unsigned long long sampleIntervalNs_;
};

class ProfHostPidCpuJob : public ProfHostDataBase {
public:
    ProfHostPidCpuJob();
    ~ProfHostPidCpuJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
    bool IsGlobalJobLevel() override
    {
        return true;
    }
};

class ProfHostPidMemJob : public ProfHostDataBase {
public:
    ProfHostPidMemJob();
    ~ProfHostPidMemJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
    bool IsGlobalJobLevel() override
    {
        return true;
    }
};

class ProfHostSysCpuJob : public ProfHostDataBase {
public:
    ProfHostSysCpuJob();
    ~ProfHostSysCpuJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
};
 
class ProfHostSysMemJob : public ProfHostDataBase {
public:
    ProfHostSysMemJob();
    ~ProfHostSysMemJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
};

class ProfHostNetworkJob : public ProfHostDataBase {
public:
    ProfHostNetworkJob();
    ~ProfHostNetworkJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
    bool IsGlobalJobLevel() override
    {
        return true;
    }
};

class ProfHostService : public analysis::dvvp::common::thread::Thread {
public:
    ProfHostService();
    ~ProfHostService() override;
public:
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg,
                const HostTimerHandlerTag hostTimerTag);
    int Process();
    int Uninit();
    int Start() override;
    int Stop() override;

public:
    void WakeupTimeoutEnd();
    void WaitTimeoutEnd();

protected:
    volatile bool isStarted_{false};

private:
    void Run(const struct error_message::Context &errorContext) override;
    int Handler();
    int GetCollectSysCallsCmd(int pid, std::string &profHostCmd);
    int GetCollectPthreadsCmd(int pid, std::string &profHostCmd);
    int GetCollectIOTopCmd(int pid, std::string &profHostCmd);
    int WriteDone();
    int GetCmdStr(int hostSysPid, std::string &profHostCmd);
    int CollectToolIsRun();
    int WaitCollectToolStart();
    int WaitCollectToolEnd();
    void PrintFileContent(const std::string filePath) const;
    int KillToolAndWaitHostProcess() const;

private:
    std::string profHostOutDir_;
    std::string toolName_;
    HostTimerHandlerTag hostTimerTag_;
    MmProcess hostProcess_;
    uint32_t outDataNumber_{0};
    SHARED_PTR_ALIA<CollectionJobCfg> collectionJobCfg_;
    struct ProfHostWriteDoneInfo hostWriteDoneInfo_;
    std::mutex needUnintMtx_;
    std::condition_variable isJobUnint_;
    std::string startProcessCmd_;
};

class ProfHostSysCallsJob : public ProfHostDataBase {
public:
    ProfHostSysCallsJob();
    ~ProfHostSysCallsJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

private:
    SHARED_PTR_ALIA<ProfHostService> profHostService_{nullptr};
};

class ProfHostPthreadJob : public ProfHostDataBase {
public:
    ProfHostPthreadJob();
    ~ProfHostPthreadJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

private:
    SHARED_PTR_ALIA<ProfHostService> profHostService_{nullptr};
};

class ProfHostDiskJob : public ProfHostDataBase {
public:
    ProfHostDiskJob();
    ~ProfHostDiskJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;

private:
    SHARED_PTR_ALIA<ProfHostService> profHostService_{nullptr};
};

class ProfHostAllPidCpuJob : public ProfHostDataBase {
public:
    ProfHostAllPidCpuJob();
    ~ProfHostAllPidCpuJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
};

class ProfHostAllPidMemJob : public ProfHostDataBase {
public:
    ProfHostAllPidMemJob();
    ~ProfHostAllPidMemJob() override;
    int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) override;
    int Process() override;
    int Uninit() override;
};
}}}
#endif
