/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_control.h
 * Description        : 流程控制类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#ifndef ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_CONTROL_H
#define ANALYSIS_INFRASTRUCTURE_PROCESS_INCLUDE_PROCESS_CONTROL_H

#include <map>
#include <vector>
#include <functional>
#include <memory>

#include "analysis/csrc/infrastructure/process/include/process_register.h"
#include "analysis/csrc/utils/thread_pool.h"

namespace Analysis {

namespace Infra {

struct ProcessStatistcs {
    std::string processName;  // 以后升级到C++17或更高版本，可以使用std::string_view代替std::string
    std::vector<std::string> dependProcessNames;
    uint32_t returnCode;
    uint64_t startTime;
    uint64_t duration;  // 单位：微秒(us)
    bool mandatory;  // 是否为关键流程，关键流程失败后，会停止后续流程导致整体失败。true表示关键流程
    bool dfxStop;  // DFX需求：可以指定执行完哪个Process停止
};

struct OneLevelStat {
    bool generalResult;
    std::vector<ProcessStatistcs> processStatistcs;
};

struct ExecuteProcessStat {
    uint32_t chipId;
    std::vector<OneLevelStat> allLevelStat;
};


void RecordProcessStat(const ExecuteProcessStat& stat, const std::string& subDir, std::string& log);


/**
 * @brief 流程控制类
 * 本类用于控制用户通过Register注册的所有流程。支持同一个level并发运行，并提供一些DFX功能
 */
class ProcessControl final {
public:
    explicit ProcessControl(ProcessCollection& processes)
        : allProcess_(std::move(processes)) {}
    ~ProcessControl() = default;

    bool ExecuteProcess(DataInventory& dataInventory, const Context& context);

    // 获取运行结果 key为level, value为统计结构
    ExecuteProcessStat GetExecuteStat() const;

    bool VerifyProcess(const ProcessCollection& chipRelatedProcess) const;

private:
    bool RunProcesses(ProcessCollection& chipRelatedProcess, DataInventory& dataInventory, const Context& context);
    bool GetStatistician(std::vector<ProcessStatistcs>&& statistics, bool& dfxStop);
    ProcessCollection TakeAwayPreparedProcess(ProcessCollection& chipRelatedProcess) const;
    void ReleaseNoLongerUsedData(const ProcessCollection& chipRelatedProcess, DataInventory& dataInventory,
                                 size_t levelIndex);
    void RunPreparedProcess(ProcessCollection &preparedProcess, std::vector<ProcessStatistcs> &stat,
                            Analysis::Utils::ThreadPool &pool, DataInventory& dataInventory,
                            const Context& context) const;
private:
    ProcessCollection allProcess_;
    ExecuteProcessStat stat_;  // dfx: 统计运行结果
    uint32_t chipId_{};  // dfx: 记录运行什么芯片ID
};

}

}

#endif