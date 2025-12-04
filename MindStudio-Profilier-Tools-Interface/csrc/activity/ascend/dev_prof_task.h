/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : dev_prof_task.h
 * Description        : Collection Job.
 * Author             : msprof team
 * Creation Date      : 2024/07/27
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITY_ASCEND_DEV_PROF_TASK_H
#define MSPTI_ACTIVITY_ASCEND_DEV_PROF_TASK_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "csrc/common/context_manager.h"
#include "csrc/common/inject/inject_base.h"
#include "csrc/include/mspti_activity.h"
#include "csrc/include/mspti_result.h"

namespace Mspti {
namespace Ascend {

class DevProfTask {
public:
    explicit DevProfTask() = default;
    virtual ~DevProfTask() = default;
    msptiResult Start();
    msptiResult Stop();

private:
    void Run();
    virtual msptiResult StartTask() = 0;
    virtual msptiResult StopTask() = 0;

private:
    std::thread t_;
    std::condition_variable cv_;
    std::mutex cv_mtx_;
    bool task_run_{false};
};

class DevProfTaskDefault : public DevProfTask {
public:
    DevProfTaskDefault(uint32_t deviceId) : deviceId_(deviceId) {}

private:
    msptiResult StartTask() override {return MSPTI_SUCCESS;}
    msptiResult StopTask() override {return MSPTI_SUCCESS;}

private:
    uint32_t deviceId_;
};

class DevProfTaskTsFw : public DevProfTask {
public:
    DevProfTaskTsFw(uint32_t deviceId) : deviceId_(deviceId) {};

private:
    msptiResult StartTask() override;
    msptiResult StopTask() override;

private:
    AI_DRV_CHANNEL channelId_ = PROF_CHANNEL_TS_FW;
    uint32_t deviceId_;
    static std::map<uint32_t, uint32_t> ref_cnts_;
    static std::mutex cnt_mtx_;
};

class DevProfTaskStars : public DevProfTask {
public:
    DevProfTaskStars(uint32_t deviceId) : deviceId_(deviceId) {};

private:
    msptiResult StartTask() override;
    msptiResult StopTask() override;

private:
    AI_DRV_CHANNEL channelId_ = PROF_CHANNEL_STARS_SOC_LOG;
    uint32_t deviceId_;
    static std::map<uint32_t, uint32_t> ref_cnts_;
    static std::mutex cnt_mtx_;
};

class DevProfTaskFactory {
public:
    static std::vector<std::unique_ptr<DevProfTask>> CreateTasks(uint32_t deviceId, msptiActivityKind kind);

private:
    static std::unique_ptr<DevProfTask> CreateDevChannelTask(uint32_t deviceId, AI_DRV_CHANNEL channelId);

private:
    // <deviceId, ChannelID>
    const static std::map<Mspti::Common::PlatformType,
        std::map<msptiActivityKind, std::set<AI_DRV_CHANNEL>>> kindToChannel_map_;
};

}  // Ascend
}  // Mspti
#endif
