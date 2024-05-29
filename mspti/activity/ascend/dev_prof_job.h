/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : dev_prof_job.h
 * Description        : Collection Job.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITY_ASCEND_DEV_PROF_JOB_H
#define MSPTI_ACTIVITY_ASCEND_DEV_PROF_JOB_H

#include <memory>

#include "common/inject/inject_base.h"
#include "external/mspti_activity.h"
#include "external/mspti_base.h"

namespace Mspti {
namespace Ascend {
class DevProfJob {
public:
    DevProfJob() = default;
    virtual ~DevProfJob() = default;
    virtual msptiResult Start() = 0;
    virtual msptiResult Stop() = 0;
    msptiResult AddReader(uint32_t deviceId, AI_DRV_CHANNEL channelId);
    msptiResult RemoveReader(uint32_t deviceId, AI_DRV_CHANNEL channelId);
};

class ProfTsTrackJob : public DevProfJob {
public:
    ProfTsTrackJob(uint32_t deviceId) : deviceId_(deviceId) {};
    ~ProfTsTrackJob() = default;
    msptiResult Start() override;
    msptiResult Stop() override;

protected:
    AI_DRV_CHANNEL channelId_ = PROF_CHANNEL_TS_FW;
    uint32_t deviceId_;
};

class CollectionJobFactory {
public:
    static std::shared_ptr<DevProfJob> CreateJob(uint32_t deviceId, msptiActivityKind kind);
};
}  // Ascend
}  // Mspti

#endif
