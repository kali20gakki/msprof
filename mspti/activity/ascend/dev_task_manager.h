
/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : dev_task_manager.h
 * Description        : Manager all ascend device prof task.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITY_ASCEND_ASCEND_MANAGER_H
#define MSPTI_ACTIVITY_ASCEND_ASCEND_MANAGER_H

#include <memory>
#include <mutex>
#include <map>
#include <set>
#include <vector>

#include "activity/ascend/dev_prof_task.h"
#include "external/mspti_result.h"

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
    msptiResult StartAllDevKindProfTask(std::vector<std::unique_ptr<DevProfTask>>& profTasks);
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
