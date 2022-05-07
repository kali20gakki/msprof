/**
 * @file collection_register.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef ANALYSIS_DVVP_JOB_WRAPPER_COLLECTION_REGISTER_H
#define ANALYSIS_DVVP_JOB_WRAPPER_COLLECTION_REGISTER_H

#include <memory>
#include "errno/error_code.h"
#include "message/prof_params.h"
#include "msprof_dlog.h"
#include "singleton/singleton.h"
#include "utils/utils.h"

namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
enum ProfCollectionJobE {
    DDR_DRV_COLLECTION_JOB = 0,
    HBM_DRV_COLLECTION_JOB,
    LLC_DRV_COLLECTION_JOB,
    DVPP_COLLECTION_JOB,
    NIC_COLLECTION_JOB,
    PCIE_DRV_COLLECTION_JOB,
    HCCS_DRV_COLLECTION_JOB,
    ROCE_DRV_COLLECTION_JOB,
    // ts
    TS_CPU_DRV_COLLECTION_JOB,
    TS_TRACK_DRV_COLLECTION_JOB,
    AIV_TS_TRACK_DRV_COLLECTION_JOB,
    AI_CORE_SAMPLE_DRV_COLLECTION_JOB,
    AI_CORE_TASK_DRV_COLLECTION_JOB,
    AIV_SAMPLE_DRV_COLLECTION_JOB,
    AIV_TASK_DRV_COLLECTION_JOB,
    HWTS_LOG_COLLECTION_JOB,
    AIV_HWTS_LOG_COLLECTION_JOB,
    FMK_COLLECTION_JOB,
    L2_CACHE_TASK_COLLECTION_JOB,
    STARS_SOC_LOG_COLLECTION_JOB,
    STARS_BLOCK_LOG_COLLECTION_JOB,
    STARS_SOC_PROFILE_COLLECTION_JOB,
    FFTS_PROFILE_COLLECTION_JOB,
    BIU_COLLECTION_JOB,
    // system
    CTRLCPU_PERF_COLLECTION_JOB,
    SYSSTAT_PROC_COLLECTION_JOB,
    SYSMEM_PROC_COLLECTION_JOB,
    ALLPID_PROC_COLLECTION_JOB,
    // host system
    HOST_CPU_COLLECTION_JOB,
    HOST_MEM_COLLECTION_JOB,
    HOST_NETWORK_COLLECTION_JOB,
    HOST_SYSCALLS_COLLECTION_JOB,
    HOST_PTHREAD_COLLECTION_JOB,
    HOST_DISKIO_COLLECTION_JOB,
    NR_MAX_COLLECTION_JOB
    //
};

static const std::string COLLECTION_JOB_FILENAME[NR_MAX_COLLECTION_JOB] = {
    "data/ddr.data",
    "data/hbm.data",
    "data/llc.data",
    "data/dvpp.data",
    "data/nic.data",
    "data/pcie.data",
    "data/hccs.data",
    "data/roce.data",
    "data/tscpu.data",
    "data/ts_track.data",
    "data/ts_track.aiv_data",
    "data/aicore.data",
    "data/aicore.data",
    "data/aiVectorCore.data",
    "data/aiVectorCore.data",
    "data/hwts.data",
    "data/hwts.aiv_data",
    "data/training_trace.data",
    "data/l2_cache.data",
    "data/stars_soc.data",
    "data/stars_block.data",
    "data/stars_soc_profile.data",
    "data/ffts_profile.data",
    "data/biu",
    "data/ai_ctrl_cpu.data",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    ""
};

struct CollectionJobParams {
    int coreNum;
    ProfCollectionJobE jobTag;
    std::string dataPath;
    SHARED_PTR_ALIA<std::vector<int>> cores;
    SHARED_PTR_ALIA<std::vector<std::string>> events;
    SHARED_PTR_ALIA<std::vector<int>> aivCores;
    SHARED_PTR_ALIA<std::vector<std::string>> aivEvents;
};

struct CollectionJobCommonParams {
    int devId;
    int devIdOnHost;
    int devIdFlush;
    std::string tmpResultDir;
    SHARED_PTR_ALIA<analysis::dvvp::message::ProfileParams> params;
    SHARED_PTR_ALIA<analysis::dvvp::message::JobContext> jobCtx;
};

struct CollectionJobCfg {
    CollectionJobParams jobParams;
    SHARED_PTR_ALIA<CollectionJobCommonParams> comParams;
};

class ICollectionJob;
struct CollectionJobT {
    CollectionJobT()
        : jobTag(NR_MAX_COLLECTION_JOB),
          jobCfg(nullptr),
          collectionJob(nullptr)
    {
    }
    Analysis::Dvvp::JobWrapper::ProfCollectionJobE jobTag;
    SHARED_PTR_ALIA<Analysis::Dvvp::JobWrapper::CollectionJobCfg> jobCfg;
    SHARED_PTR_ALIA<Analysis::Dvvp::JobWrapper::ICollectionJob> collectionJob;
};

class ICollectionJob {
public:
    virtual ~ICollectionJob();
    virtual int Init(const SHARED_PTR_ALIA<CollectionJobCfg> cfg) = 0;
    virtual int Process() = 0;
    virtual int Uninit() = 0;
    virtual bool IsGlobalJobLevel() { return false; };
};

class CollectionRegisterMgr : public analysis::dvvp::common::singleton::Singleton<CollectionRegisterMgr> {
public:
    CollectionRegisterMgr();
    virtual ~CollectionRegisterMgr();

public:
    int CollectionJobRegisterAndRun(int devId,
                                    const ProfCollectionJobE jobTag,
                                    const SHARED_PTR_ALIA<ICollectionJob> job);
    int CollectionJobUnregisterAndStop(int devId, const ProfCollectionJobE jobTag);

private:
    bool CheckCollectionJobIsNoRegister(int &devId, const ProfCollectionJobE jobTag) const;
    bool InsertCollectionJob(int devId, const ProfCollectionJobE jobTag, const SHARED_PTR_ALIA<ICollectionJob> job);
    bool GetAndDelCollectionJob(int devId, const ProfCollectionJobE jobTag, SHARED_PTR_ALIA<ICollectionJob> &job);

private:
    std::map<int, std::map<ProfCollectionJobE, SHARED_PTR_ALIA<ICollectionJob>>> collectionJobs_;
    std::mutex collectionJobsMutex_;
};
}}}
#endif