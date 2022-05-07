/**
* @file collection_register.cpp
*
* Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

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
 * @berif  : jugde the job is exist or not
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
 * @berif  : register the job and run the job
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
        MSPROF_LOGD("Collection Job Registeter, devId:%d jobTag:%d", devId, jobTag);
        return job->Process();
    }

    return PROFILING_FAILED;
}

/**
 * @berif  : unregister the job and uninit the job
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
        MSPROF_LOGD("Collection Job Unregisteter, devId:%d jobTag:%d", devId, jobTag);
        if (job != nullptr) {
            return job->Uninit();
        }
    }

    return PROFILING_FAILED;
}

/**
 * @berif  : insert collection job
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
 * @berif  : get collection job and delete the job
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
