
/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : ascend_manager.h
 * Description        : Manager all ascend device prof task.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_ACTIVITY_ASCEND_ASCEND_MANAGER_H
#define MSPTI_ACTIVITY_ASCEND_ASCEND_MANAGER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "activity/ascend/device.h"
#include "external/mspti_base.h"

namespace Mspti {
namespace Ascend {

// Singleton
class AscendManager {
public:
    static AscendManager* GetInstance();
    msptiResult StartDevProfTask(uint32_t deviceId);
    msptiResult StopDevProfTask(uint32_t deviceId);

private:
    AscendManager() = default;
    ~AscendManager();
    explicit AscendManager(const AscendManager &obj) = delete;
    AscendManager& operator=(const AscendManager &obj) = delete;
    explicit AscendManager(AscendManager &&obj) = delete;
    AscendManager& operator=(AscendManager &&obj) = delete;

private:
    std::unordered_map<int32_t, std::unique_ptr<Device>> dev_map_;
    std::mutex dev_map_mtx_;
};

inline void AscendProfEnable(bool enable, int32_t device)
{
    enable ? Mspti::Ascend::AscendManager::GetInstance()->StartDevProfTask(static_cast<uint32_t>(device)) :
        Mspti::Ascend::AscendManager::GetInstance()->StopDevProfTask(static_cast<uint32_t>(device));
}

}  // Ascend
}  // Mspti

#endif
