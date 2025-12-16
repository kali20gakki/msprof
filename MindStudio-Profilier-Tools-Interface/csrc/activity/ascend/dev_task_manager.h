
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

#ifndef MSPTI_ACTIVITY_ASCEND_ASCEND_MANAGER_H
#define MSPTI_ACTIVITY_ASCEND_ASCEND_MANAGER_H

#include <memory>
#include <mutex>
#include <map>
#include <set>
#include <vector>

#include "csrc/activity/ascend/dev_prof_task.h"
#include "csrc/include/mspti_result.h"

namespace Mspti {
namespace Ascend {

// Singleton
class DevTaskManager {
public:
    using ActivitySwitchType = std::array<std::atomic<bool>, MSPTI_ACTIVITY_KIND_COUNT>;

    static DevTaskManager* GetInstance();
    msptiResult StartDevProfTask(uint32_t deviceId, const ActivitySwitchType& kinds);
    msptiResult StopDevProfTask(uint32_t deviceId, const ActivitySwitchType& kinds);
    bool CheckDeviceOnline(uint32_t deviceId);
    void RegisterReportCallback();

private:
    DevTaskManager();
    ~DevTaskManager();
    explicit DevTaskManager(const DevTaskManager &obj) = delete;
    DevTaskManager& operator=(const DevTaskManager &obj) = delete;
    explicit DevTaskManager(DevTaskManager &&obj) = delete;
    DevTaskManager& operator=(DevTaskManager &&obj) = delete;
    void InitDeviceList();
    msptiResult StartAllDevKindProfTask(std::vector<std::unique_ptr<DevProfTask>>& profTasks,
                                        std::vector<std::unique_ptr<DevProfTask>>& successTasks);
    msptiResult StopAllDevKindProfTask(std::vector<std::unique_ptr<DevProfTask>>& profTasks);

    msptiResult StartCannProfTask(uint32_t deviceId, const ActivitySwitchType& kinds);
    msptiResult StopCannProfTask(uint32_t deviceId);

private:
    std::set<uint32_t> device_set_;
    std::once_flag get_device_flag_;
    static std::map<msptiActivityKind, uint64_t> datatype_config_map_;

    std::map<std::pair<uint32_t, msptiActivityKind>, std::vector<std::unique_ptr<DevProfTask>>> task_map_;
    std::mutex task_map_mtx_;
    std::atomic<uint64_t> profSwitch_{0};
};
}  // Ascend
}  // Mspti

#endif
