/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : host_trace_worker.h
 * Description        : HostTraceWorker模块：
 *                      负责拉起数据解析->EventQueue->建树->分析树->Dump 流程
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_WORKER_HOST_TRACE_THREAD_H
#define ANALYSIS_WORKER_HOST_TRACE_THREAD_H
#include <string>
#include <utility>
#include <set>
#include "analysis/csrc/parser/host/cann/cann_warehouse.h"
#include "analysis/csrc/utils/safe_unordered_map.h"
#include "analysis/csrc/entities/tree.h"

namespace Analysis {
namespace Worker {

// HostTraceWorker负责控制CANN Host侧数据解析->EventQueue->建树->分析树->Dump 流程
// 此流程由Worker拉起
class HostTraceWorker {
    using TreeNode = Analysis::Entities::TreeNode;
    using CANNWarehouse = Analysis::Parser::Host::Cann::CANNWarehouse;
    using CANNWarehouses = Analysis::Utils::SafeUnorderedMap<uint32_t, CANNWarehouse>;
public:
    // hostPath为采集侧落盘host二进制数据路径
    explicit HostTraceWorker(const std::string &hostPath)
        : hostPath_(hostPath)
    {}
    // 启动整个流程
    bool Run();
private:
    void SortEvents();
    void MultiThreadBuildTree();
    void MultiThreadAnalyzeTreeDumpData();
private:
    const uint32_t poolSize_ = 10;
    std::mutex mutex_;
    std::vector<std::pair<uint32_t, std::shared_ptr<TreeNode>>> treeNodes_;
    std::set<uint32_t> threadIds_;
    std::string hostPath_;
    CANNWarehouses cannWarehouses_;
};

} // namespace Worker
} // namespace Analysis
#endif // ANALYSIS_WORKER_HOST_TRACE_THREAD_H
