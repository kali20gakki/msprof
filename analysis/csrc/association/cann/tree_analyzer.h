/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_analyzer.h
 * Description        : Tree分析模块：遍历Tree生成待落盘的HostTask
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H
#define ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H

#include <memory>
#include <queue>
#include <map>
#include <unordered_map>
#include "tree.h"
#include "ascend_obj.h"

namespace Analysis {
namespace Association {
namespace Cann {

using HostTask = Analysis::Entities::HostTask;
using Operator = Analysis::Entities::Operator;
using Tree = Analysis::Entities::Tree;
using TreeNode = Analysis::Entities::TreeNode;

using HostTasks = std::vector<std::shared_ptr<HostTask>>;
using HCCLBigOps = std::unordered_map<uint32_t, std::vector<std::shared_ptr<Operator>>>;

class TreeAnalyzer {
public:
    explicit TreeAnalyzer(const std::shared_ptr<Tree> &tree);
    // 入口函数
    void Analyze();
    // 获取HCCL相关Tasks数据
    HostTasks &GetHCCLTasks();
    // 获取计算类Tasks数据
    HostTasks &GetComputeTasks();
    // 获取所有Tasks数据
    HostTasks &GetTasks();
    // 获取HCCL大算子数据
    HCCLBigOps &GetHcclBigOps();

private:
    uint32_t threadId_ = 0; // 此对象处理的threadId
    std::shared_ptr<Tree> tree_;
    std::map<uint16_t, std::shared_ptr<TreeNode>> path_; // 路径记录
    HostTasks tasks_;
    HostTasks hcclTasks_;
    HostTasks kernelTasks_;
    HCCLBigOps hcclBigOps_;
};
} // namespace Cann
} // namespace Association
} // namespace Analysis
#endif // ANALYSIS_ASSOCIATION_CANN_TREE_ANALYZER_H
