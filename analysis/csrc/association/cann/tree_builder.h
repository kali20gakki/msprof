/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree_builder.h
 * Description        : EventTree建树模块：遍历EventQueue建树
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H
#define ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H

#include <memory>
#include <utility>
#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>
#include "event_queue.h"
#include "tree.h"

namespace Analysis {
namespace Association {
namespace Cann {

// 一个TreeBuilder对象负责将一个threadId的Event数据建成一棵树
class TreeBuilder {
public:
    // 用api和task_track建核心树
    std::shared_ptr<TreeNode> Build(std::shared_ptr<EventQueue> &kernelEvents);

    // 向核心树的Model, Node, Hccl, Runtime 层节点添加附加Event
    bool AddModelLevelEvents(std::shared_ptr<EventQueue> &events);
    bool AddNodeLevelEvents(std::shared_ptr<EventQueue> &events);
    bool AddHcclLevelEvents(std::shared_ptr<EventQueue> &events);
    bool AddRuntimeLevelEvents(std::shared_ptr<EventQueue> &events);

private:
    std::shared_ptr<TreeNode> BuildTree(std::shared_ptr<TreeNode> parent); // 建树逻辑
    std::shared_ptr<EventQueue> events_; // 某个threadId对应的EventQueue
    std::map<uint16_t, std::shared_ptr<TreeNode>> path_; // 建树路径记录，key为Event的层级

    uint32_t threadId_ = 0; // 此对象处理的threadId
    // 建树时Model, Node, Hccl, Runtime 层节点记录
    std::vector<std::shared_ptr<TreeNode>> modelLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> nodeLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> hcclLevelNodes_;
    std::vector<std::shared_ptr<TreeNode>> runtimeLevelNodes_;
};
} // namespace Cann
} // namespace Association
} // namespace Analysis
#endif // ANALYSIS_ASSOCIATION_CANN_TREE_BUILDER_H
