/* -------------------------------------------------------------------------
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This file is part of the MindStudio project.
 *
 * MindStudio is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *    http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 * -------------------------------------------------------------------------*/

#include "collection_register.h"
namespace Analysis {
namespace Dvvp {
namespace JobWrapper {
using namespace analysis::dvvp::common::error;

ICollectionJob::~ICollectionJob()
{
}

CollectionRegisterMgr::CollectionRegisterMgr()
{
}

CollectionRegisterMgr::~CollectionRegisterMgr()
{
    collectionJobs_.clear();
}

/**
 * @brief  : judge the job is exist or not
 * @param  : [in] devId   : device id
 * @param  : [in] jobTag : collection job Tag
 * @return : true : exist Job; false : not exist Job
 */
bool CollectionRegisterMgr::CheckCollectionJobIsNoRegister(int &devId, const ProfCollectionJobE jobTag) const
{
    if (devId < 0 || jobTag >= NR_MAX_COLLECTION_JOB) {
        return false;
    }
    // GLOBAL LEVEL
    for (auto iter = collectionJobs_.begin(); iter != collectionJobs_.end(); iter++) {
        auto it = iter->second.find(jobTag);
        if (it != iter->second.end() && it->second->IsGlobalJobLevel()) {
            devId = iter->first;
            MSPROF_LOGW("IsGlobalJobLevel, devId:%d jobTag:%d", iter->first, jobTag);
            return false;
        }
    }

    // DEVICE LEVEL
    auto iter = collectionJobs_.find(devId);
    if (iter == collectionJobs_.end()) {
        return true;
    }

    auto it = iter->second.find(jobTag);
    if (it == iter->second.end() || it->second == nullptr) {
        return true;
    }

    return false;
}

/**
 * @brief  : register the job and run the job
 * @param  : [in] devId   : device id
 * @param  : [in] jobTag : collection job Tag
 * @return : true : exist Job; false : not exist Job
 */
int CollectionRegisterMgr::CollectionJobRegisterAndRun(int devId,
    const ProfCollectionJobE jobTag, const SHARED_PTR_ALIA<ICollectionJob> job)
{
    if (devId < 0 || jobTag >= NR_MAX_COLLECTION_JOB || job == nullptr) {
        return PROFILING_FAILED;
    }
    std::lock_guard<std::mutex> lk(collectionJobsMutex_);
    if (InsertCollectionJob(devId, jobTag, job)) {
        MSPROF_LOGD("Collection Job Register, devId:%d jobTag:%d", devId, jobTag);
        return job->Process();
    }

    return PROFILING_FAILED;
}

int CollectionRegisterMgr::CollectionJobRun(int32_t devId, const ProfCollectionJobE jobTag)
{
    if (devId < 0 || jobTag >= NR_MAX_COLLECTION_JOB) {
        return PROFILING_FAILED;
    }

    std::lock_guard<std::mutex> lk(collectionJobsMutex_);
    if (collectionJobs_.find(devId) == collectionJobs_.end() ||
        collectionJobs_[devId].find(jobTag) == collectionJobs_[devId].end()) {
        MSPROF_LOGI("Collection job not register, devId:%d jobTag:%d", devId, jobTag);
        return PROFILING_FAILED;
    }
    return collectionJobs_[devId][jobTag]->Process();
}

/**
 * @brief  : unregister the job and uninit the job
 * @param  : [in] devId   : device id
 * @param  : [in] jobTag : collection job Tag
 * @return : true : exist Job; false : not exist Job
 */
int CollectionRegisterMgr::CollectionJobUnregisterAndStop(int devId, const ProfCollectionJobE jobTag)
{
    if (devId < 0 || jobTag >= NR_MAX_COLLECTION_JOB) {
        return PROFILING_FAILED;
    }
    SHARED_PTR_ALIA<ICollectionJob> job = nullptr;
    std::lock_guard<std::mutex> lk(collectionJobsMutex_);
    if (GetAndDelCollectionJob(devId, jobTag, job)) {
        MSPROF_LOGD("Collection Job Unregister, devId:%d jobTag:%d", devId, jobTag);
        if (job != nullptr) {
            return job->Uninit();
        }
    }

    return PROFILING_FAILED;
}

/**
 * @brief  : insert collection job
 * @param  : [in] devId   : device id
 * @param  : [in] jobTag : collection job Tag
 * @param  : [in] SHARED_PTR_ALIA<ICollectionJob> job : collection job
 * @return : true : exist Job; false : not exist Job
 */
bool CollectionRegisterMgr::InsertCollectionJob(int devId,
    const ProfCollectionJobE jobTag, const SHARED_PTR_ALIA<ICollectionJob> job)
{
    if (devId < 0 || jobTag >= NR_MAX_COLLECTION_JOB || job == nullptr) {
        return false;
    }
    std::map<ProfCollectionJobE, SHARED_PTR_ALIA<ICollectionJob>> colJob;
    // no register
    if (CheckCollectionJobIsNoRegister(devId, jobTag)) {
        auto iter = collectionJobs_.find(devId);
        if (iter != collectionJobs_.end()) {
            colJob = collectionJobs_[devId];
        }

        colJob[jobTag] = job;
        collectionJobs_[devId] = colJob;
        return true;
    }

    return false;
}

/**
 * @brief  : get collection job and delete the job
 * @param  : [in] devId   : device id
 * @param  : [in] jobTag : collection job Tag
 * @param  : [out] SHARED_PTR_ALIA<ICollectionJob> job : collection job
 * @return : true : exist Job; false : not exist Job
 */
bool CollectionRegisterMgr::GetAndDelCollectionJob(int devId,
    const ProfCollectionJobE jobTag, SHARED_PTR_ALIA<ICollectionJob> &job)
{
    if (devId < 0 || jobTag >= NR_MAX_COLLECTION_JOB) {
        return false;
    }

    int checkId = devId;
    if (!CheckCollectionJobIsNoRegister(checkId, jobTag) && checkId == devId) {
        auto collectionJob = collectionJobs_[devId];
        job = collectionJob[jobTag];
        collectionJob.erase(jobTag);
        collectionJobs_[devId] = collectionJob;
        return true;
    }

    return false;
}
}}}
