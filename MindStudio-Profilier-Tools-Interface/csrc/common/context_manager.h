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

#ifndef MSPTI_COMMON_CONTEXT_MANAGER_H
#define MSPTI_COMMON_CONTEXT_MANAGER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <thread>
#include <vector>
#include "csrc/include/mspti_result.h"

namespace Mspti {
namespace Common {

struct DevTimeInfo {
    uint64_t freq{0};
    uint64_t startRealTime{0};
    uint64_t startSysCnt{0};
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
    bool GetHostTimeInfo(DevTimeInfo& devTimeInfo);
    bool GetCurDevTimeInfo(uint32_t deviceId, DevTimeInfo& devTimeInfo);
    static uint64_t CalculateRealTime(uint64_t sysCnt, const DevTimeInfo& devTimeInfo);
    uint64_t GetRealTimeFromSysCnt(uint32_t deviceId, uint64_t sysCnt);
    std::vector<uint64_t> GetRealTimeFromSysCnt(uint32_t deviceId, const std::vector<uint64_t>& sysCnts);
    // Host
    uint64_t GetRealTimeFromSysCnt(uint64_t sysCnt);
    uint64_t GetHostTimeStampNs();
    PlatformType GetChipType(uint32_t deviceId);
    uint64_t GetCorrelationId(uint32_t threadId = 0);
    uint64_t UpdateAndReportCorrelationId();
    uint64_t UpdateAndReportCorrelationId(uint32_t tid);

    msptiResult StartSyncTime();
    msptiResult StopSyncTime();

private:
    ContextManager() = default;
    ~ContextManager();
    explicit ContextManager(const ContextManager &obj) = delete;
    ContextManager& operator=(const ContextManager &obj) = delete;
    explicit ContextManager(ContextManager &&obj) = delete;
    ContextManager& operator=(ContextManager &&obj) = delete;

    void Run();

private:
    std::unordered_map<uint32_t, std::unique_ptr<DevTimeInfo>> devTimeInfo_;
    std::mutex devTimeMtx_;

    std::mutex hostTimeMtx_;
    std::unique_ptr<DevTimeInfo> hostTimeInfo_;

    std::atomic<bool> isQuit_{false};
    std::thread t_;

    std::mutex correlationIdMtx_;
    uint64_t correlationId_{0};

    std::mutex cv_mutex_;
    std::condition_variable cv_;

    std::unordered_map<uint32_t, uint64_t> threadCorrelationIdInfo_;
};
}  // Common
}  // Mspti

#endif
