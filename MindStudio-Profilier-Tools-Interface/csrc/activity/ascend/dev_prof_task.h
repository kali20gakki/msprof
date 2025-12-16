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
 * -------------------------------------------------------------------------
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
