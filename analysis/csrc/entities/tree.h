/* ******************************************************************************
            版权所有 (c) 华为技术有限公司 2023-2023
            Copyright, 2023, Huawei Tech. Co., Ltd.
****************************************************************************** */
/* ******************************************************************************
 * File Name          : tree.h
 * Description        : Tree模块：包括TreeNode数据结构和Tree数据结构
 * Author             : msprof team
 * Creation Date      : 2023/11/2
 * *****************************************************************************
 */

#ifndef ANALYSIS_ENTITIES_TREE_H
#define ANALYSIS_ENTITIES_TREE_H

#include <memory>
#include <map>
#include <utility>

#include "analysis/csrc/entities/event_queue.h"

namespace Analysis {
namespace Entities {

// Event树节点数据结构，用于重建调用树
struct TreeNode {
    explicit TreeNode(std::shared_ptr<Event> eventPtr) : event(std::move(eventPtr))
    {}
    std::shared_ptr<TreeNode> parent = nullptr;
    std::vector<std::shared_ptr<TreeNode>> children;
    std::shared_ptr<Event> event = nullptr;        // 表示该节点的对应的api
    std::vector<std::shared_ptr<Event>> records;   // 表示节点时间范围内的补充记录信息
};

// Event树 一个threadId一棵树
class Tree {
public:
    explicit Tree(const std::shared_ptr<TreeNode> &rootNode) : root_(rootNode)
    {}
    // 获取根节点
    std::shared_ptr<TreeNode> GetRoot() const;
    // 层序遍历打印Tree结构便于问题定位
    std::vector<std::string> Show();
private:
    std::string GetTreeLevelStr(const std::shared_ptr<TreeNode> &node) const;
    std::shared_ptr<TreeNode> root_ = nullptr;
};

} // namespace Entities
} // namespace Analysis
#endif // ANALYSIS_ENTITIES_TREE_H
