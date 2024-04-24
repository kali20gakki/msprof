/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2024-2024
            Copyright, 2024, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : process_control.cpp
 * Description        : 流程控制类
 * Author             : msprof team
 * Creation Date      : 2024/4/12
 * *****************************************************************************
 */
#include "analysis/csrc/infrastructure/process/include/process_control.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <iterator>
#include <sstream>
#include <locale>
#include <set>
#include "analysis/csrc/infrastructure/process/process_topo.h"

namespace Analysis {

namespace Infra {
namespace {

void GetDepProcessNames(const RegProcessInfo& procInfo, const ProcessCollection& allRegProcess,
                        std::vector<std::string>& depProcNames)
{
    for (const auto& typeIndex : procInfo.processDependence) {
        auto it = allRegProcess.find(typeIndex);
        if (it == allRegProcess.end()) {
            continue;
        }
        depProcNames.emplace_back(it->second.processName);
    }
}

void FillStatisticianDependence(const ProcessCollection& regProcess, std::vector<ProcessStatistcs>& stat)
{
    for (auto& node : stat) {
        for (const auto& regPair : regProcess) {
            if (node.processName == regPair.second.processName) {
                GetDepProcessNames(regPair.second, regProcess, node.dependProcessNames);
            }
        }
    }
}

}

class ProcessControl::Impl final {
public:
    explicit Impl(ProcessCollection& processes)
        : allProcess_(std::move(processes)) {}
    ~Impl() = default;

    bool VerifyProcess(const ProcessCollection& chipRelatedProcess) const
    {
        auto verifyProcess = chipRelatedProcess;
        auto preparedProcess = TakeAwayPreparedProcess(verifyProcess);
        while (!preparedProcess.empty()) {
            preparedProcess = TakeAwayPreparedProcess(verifyProcess);
        }
        return verifyProcess.empty();
    }

    bool ExecuteProcess(DataInventory& dataInventory, const Context& context)
    {
        chipId_ = context.GetChipID();
        stat_.chipId = chipId_;

        ProcessTopo processTopoBuilder(allProcess_);
        auto chipRelatedProcess = processTopoBuilder.BuildProcessControlTopoByChip(chipId_);
        if (!VerifyProcess(chipRelatedProcess)) {
            ERROR("Topo Verify failed!");
            return false;
        }

        return RunProcesses(chipRelatedProcess, dataInventory, context);
    }

    ExecuteProcessStat GetExecuteStat() const
    {
        return stat_;
    }

private:
    /**
     * @brief 拓扑排序算法执行所有Process 使用的算法为卡恩算法
     * @param chipRelatedProcess 相应Chip的Process集合
     */
    bool RunProcesses(ProcessCollection& chipRelatedProcess, DataInventory& dataInventory,
                      const Context& context)
    {
        auto regProcessCopy = chipRelatedProcess; // 这里COPY一份是为了DFX中填dependProcess字段

        size_t levelIndex = 0;
        auto preparedProcess = TakeAwayPreparedProcess(chipRelatedProcess);
        while (!preparedProcess.empty()) {
            std::vector<ProcessStatistcs> stat(preparedProcess.size());
            Analysis::Utils::ThreadPool pool(preparedProcess.size());
            pool.Start();
            RunPreparedProcess(preparedProcess, stat, pool, dataInventory, context);
            pool.WaitAllTasks();
            pool.Stop();

            // DFX
            FillStatisticianDependence(regProcessCopy, stat);
            bool dfxStop = false;
            auto ret = GetStatistician(std::move(stat), dfxStop);
            if (!ret) {
                ERROR("mandatory process failed!");
                return false;
            }
            if (dfxStop) {
                PRINT_INFO("!!!!!!! DFX Stops !!!!!!!!");
                return false;
            }
            ReleaseNoLongerUsedData(chipRelatedProcess, dataInventory, levelIndex++);
            preparedProcess = TakeAwayPreparedProcess(chipRelatedProcess);
        }
        return true;
    }

    void RunPreparedProcess(ProcessCollection &preparedProcess, std::vector<ProcessStatistcs> &stat,
                            Analysis::Utils::ThreadPool &pool, DataInventory& dataInventory,
                            const Context& context) const
    {
        size_t concurrentIndex{};
        for (const auto& processNode : preparedProcess) {
            pool.AddTask([&processNode, &stat, &dataInventory, &context, concurrentIndex]() {
                auto startTime = std::chrono::steady_clock::now();
                if (!processNode.second.creator) {
                    ERROR("creator==nullptr, process name: %", processNode.second.processName);
                    return;
                }
                auto proc = processNode.second.creator();
                if (proc != nullptr) {
                    stat[concurrentIndex].returnCode = proc->Run(dataInventory, context);
                    stat[concurrentIndex].mandatory = processNode.second.mandatory;
                }
                auto endTime = std::chrono::steady_clock::now();

                stat[concurrentIndex].startTime = std::chrono::duration_cast<std::chrono::microseconds>(
                        startTime.time_since_epoch()).count();
                stat[concurrentIndex].duration = std::chrono::duration_cast<std::chrono::microseconds>(
                        endTime - startTime).count();
                stat[concurrentIndex].processName = processNode.second.processName;
                stat[concurrentIndex].dfxStop = false;
                const std::string& stopProcessName = context.GetDfxStopAtName();
                if (!stopProcessName.empty() && stopProcessName == processNode.second.processName) {
                    stat[concurrentIndex].dfxStop = true;
                }
            });
            ++concurrentIndex;
        }
    }

    bool GetStatistician(std::vector<ProcessStatistcs>&& statistics, bool& dfxStop)
    {
        auto generalResult = std::all_of(statistics.begin(), statistics.end(), [](ProcessStatistcs& node) {
            return (node.returnCode == 0 || !node.mandatory);
        });
        dfxStop = std::any_of(statistics.begin(), statistics.end(), [](ProcessStatistcs& node) {
            return node.dfxStop;
        });
        stat_.allLevelStat.emplace_back();
        auto& oneLevelStat = stat_.allLevelStat.back();
        oneLevelStat.generalResult = generalResult;
        oneLevelStat.processStatistcs = std::move(statistics);
        return generalResult; // Stop on error  这里是否停止还与Process类注册时，注册宏中mandatory字段确定
    }

    void ReleaseNoLongerUsedData(const ProcessCollection& chipRelatedProcess, DataInventory& dataInventory,
                                 size_t levelIndex)
    {
        std::set<std::type_index> dataTobeUsing;
        for (const auto& processInfo : chipRelatedProcess) {
            for (const auto& dataType : processInfo.second.paramTypes) {
                dataTobeUsing.insert(dataType);
            }
        }
        auto removedTypes = dataInventory.RemoveRestData(dataTobeUsing);

        std::string typeStr;
        for (const auto& typeInfo : removedTypes) {
            typeStr += typeInfo.name();
            typeStr += " ";
        }
        INFO("Level[%]Release Data Types: %", levelIndex, typeStr);
    }

    ProcessCollection TakeAwayPreparedProcess(ProcessCollection& chipRelatedProcess) const
    {
        ProcessCollection preparedProcess;
        // 先拿走已经没有前向依赖的，即入度为0的节点
        for (auto it = chipRelatedProcess.begin(); it != chipRelatedProcess.end();) {
            if (it->second.processDependence.empty()) {
                preparedProcess.insert({it->first, it->second});
                it = chipRelatedProcess.erase(it);
                continue;
            }
            ++it;
        }

        // 再将其它节点中的前向依赖删除，即修正其它节点的入度
        for (auto& process : chipRelatedProcess) {
            auto& processDep = process.second.processDependence;
            for (const auto& preparedNode : preparedProcess) {
                processDep.erase(std::remove(processDep.begin(), processDep.end(), preparedNode.first),
                                 processDep.end());
            }
        }
        return preparedProcess;
    }

private:
    ProcessCollection allProcess_;
    ExecuteProcessStat stat_;  // dfx: 统计运行结果
    uint32_t chipId_{};  // dfx: 记录运行什么芯片ID
};

void RecordProcessStat(const ExecuteProcessStat& stat, const std::string& subDir, std::string& log)
{
    std::stringstream ss;
    ss.imbue(std::locale(""));
    ss << "===============================================================================" << std::endl;
    ss << "deivce file in " << subDir << std::endl;

    auto levelCount = stat.allLevelStat.size();
    ss << "chip id: " << stat.chipId << ", process levels:" << levelCount << std::endl;
    for (size_t i = 0; i < levelCount; ++i) {
        const auto& node = stat.allLevelStat[i];
        ss << "-------------------------------------------------------------------------------" << std::endl;
        ss << "level[" << i << "] generalResult:" << std::boolalpha << node.generalResult << std::noboolalpha
            << ", process num:" << node.processStatistcs.size() << std::endl;
        size_t j = 0;
        for (const auto& proc : node.processStatistcs) {
            ss << "\tprocess" << j++ << "[" << proc.processName << "]:" << "return: 0x"
                << std::hex << proc.returnCode << std::dec << ", mandatory:"
                << std::boolalpha << proc.mandatory << std::noboolalpha << std::endl;
            ss << "\t\tdependProcessNames: ";
            for (const auto& depName : proc.dependProcessNames) {
                ss << depName << ", ";
            }
            ss << std::endl << "\t\tstartTime:" << proc.startTime << " us, duration: "
                << proc.duration << " us" << std::endl;
        }
    }
    ss << std::endl;
    log += ss.str();
}

ProcessControl::ProcessControl(ProcessCollection& processes)
    : impl_(new Impl(processes)) {}

ProcessControl::~ProcessControl() = default;

bool ProcessControl::ExecuteProcess(DataInventory& dataInventory, const Context& context)
{
    if (!impl_) {
        ERROR("ProcessControl Impl create failed!");
        return false;
    };
    return impl_->ExecuteProcess(dataInventory, context);
}

// 获取运行结果 key为level, value为统计结构
ExecuteProcessStat ProcessControl::GetExecuteStat() const
{
    if (!impl_) {
        ERROR("ProcessControl Impl create failed!");
        return {};
    };
    return impl_->GetExecuteStat();
}

bool ProcessControl::VerifyProcess(const ProcessCollection& chipRelatedProcess) const
{
    if (!impl_) {
        ERROR("ProcessControl Impl create failed!");
        return false;
    };
    return impl_->VerifyProcess(chipRelatedProcess);
}

}

}