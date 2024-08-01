
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

#include "activity/ascend/dev_prof_task.h"
#include "external/mspti_base.h"

namespace Mspti {
namespace Ascend {

// Singleton
class DevTaskManager {
public:
    static DevTaskManager* GetInstance();
    msptiResult StartDevProfTask(uint32_t deviceId, msptiActivityKind kind);
    msptiResult StopDevProfTask(uint32_t deviceId, msptiActivityKind kind);
    bool CheckDeviceOnline(uint32_t deviceId);

private:
    DevTaskManager();
    ~DevTaskManager();
    explicit DevTaskManager(const DevTaskManager &obj) = delete;
    DevTaskManager& operator=(const DevTaskManager &obj) = delete;
    explicit DevTaskManager(DevTaskManager &&obj) = delete;
    DevTaskManager& operator=(DevTaskManager &&obj) = delete;
    void InitDeviceList();

private:
    std::set<uint32_t> device_set_;
    std::once_flag get_device_flag_;

    std::map<std::pair<uint32_t, msptiActivityKind>, std::unique_ptr<DevProfTask>> task_map_;
    std::mutex task_map_mtx_;
};
}  // Ascend
}  // Mspti

#endif
