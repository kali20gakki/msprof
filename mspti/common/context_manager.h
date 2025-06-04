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

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace Mspti {
namespace Common {

struct DevTimeInfo {
    uint64_t freq;
    uint64_t startRealTime;
    uint64_t startSysCnt;
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
    void InitHostTimeInfo();
    bool HostFreqIsEnable();
    uint64_t GetRealTimeFromSysCnt(uint32_t deviceId, uint64_t sysCnt);
    // Host
    uint64_t GetRealTimeFromSysCnt(uint64_t sysCnt);
    uint64_t GetHostTimeStampNs();
    PlatformType GetChipType(uint32_t deviceId);
    uint64_t GetCorrelationId(uint32_t threadId = 0);
    void UpdateAndReportCorrelationId();
    void UpdateAndReportCorrelationId(uint32_t tid);

private:
    ContextManager() = default;
    explicit ContextManager(const ContextManager &obj) = delete;
    ContextManager& operator=(const ContextManager &obj) = delete;
    explicit ContextManager(ContextManager &&obj) = delete;
    ContextManager& operator=(ContextManager &&obj) = delete;

private:
    std::unordered_map<uint32_t, std::unique_ptr<DevTimeInfo>> devTimeInfo_;
    std::mutex devTimeMtx_;
    std::unique_ptr<DevTimeInfo> hostTimeInfo_;

    std::mutex correlationIdMtx_;
    uint64_t correlationId_{0};
    std::unordered_map<uint32_t, uint64_t> threadCorrelationIdInfo_;
};
}  // Common
}  // Mspti

#endif
