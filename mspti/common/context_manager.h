/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : context_manager.h
 * Description        : Global context manager.
 * Author             : msprof team
 * Creation Date      : 2024/05/07
 * *****************************************************************************
*/

#ifndef MSPTI_COMMON_CONTEXT_MANAGER_H
#define MSPTI_COMMON_CONTEXT_MANAGER_H

#include <memory>
#include <mutex>
#include <unordered_map>

namespace Mspti {
namespace Common {

struct DevTimeInfo {
    uint64_t freq;
    uint64_t hostStartMonoRawTime;
    uint64_t devStartSysCnt;
};

enum class PlatformType {
    CHIP_910B = 5,
    CHIP_310B = 7,
    END_TYPE
};

class ContextManager final {
public:
    static ContextManager* GetInstance();
    void InitDevTimeInfo(uint32_t deviceId);
    uint64_t GetMonotomicFromSysCnt(uint32_t deviceId, uint64_t sysCnt);
    PlatformType GetChipType(uint32_t deviceId);

private:
    ContextManager() = default;
    explicit ContextManager(const ContextManager &obj) = delete;
    ContextManager& operator=(const ContextManager &obj) = delete;
    explicit ContextManager(ContextManager &&obj) = delete;
    ContextManager& operator=(ContextManager &&obj) = delete;

private:
    std::unordered_map<uint32_t, std::unique_ptr<DevTimeInfo>> dev_time_info_;
    std::mutex dev_time_mtx_;
};
}  // Common
}  // Mspti

#endif
